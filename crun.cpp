#include <cstdio>
#include <iterator>
#include <sys/ioctl.h>
#include <unistd.h>

import std;

import subprocess;
import nlohmann.json;
import clip;

using json = nlohmann::json;

using clip::Argument;
using clip::Command;
using clip::flag;
using clip::none;
using clip::positional;
using clip::Subcommand;

using Str = std::string;

namespace fs = std::filesystem;

namespace sp = subprocess;
using json = nlohmann::json;

constexpr auto verbosity =
    Argument<int, flag, "v", "verbose", "Level of verbosity">{};

constexpr auto compdb =
    Argument<bool, flag, none, "compdb",
             "Enables the output of compile_commands.json">{};

constexpr auto compdb_path =
    Argument<Str, flag, none, "compdb-path", "Where to output compdb">{};

constexpr auto run =
    Subcommand<none, "run", "Run program">{}
        .arg(clip::help_arg)
        .arg(Argument<Str, positional, none, "script", "Script to run">{})
        .arg(verbosity)
        .arg(compdb)
        .arg(compdb_path);

constexpr auto build =
    Subcommand<"b", "build", "Builds a script">{}
        .arg(clip::help_arg)
        .arg(verbosity)
        .arg(Argument<Str, flag, none, "build-flags", "Flags to build with">{})
        .arg(compdb)
        .arg(compdb_path)
        .arg(Argument<bool, flag, none, "rebuild",
                      "Forcibly rebuilds everything">{})
        .arg(Argument<bool, flag, none, "configure",
                      "Only configures instead of building anything">{})
        .arg(Argument<Str, positional, none, "script", "Script to build">{});

constexpr auto reg =
    Subcommand<"reg", "register", "Registers a command into PATH">{}
        .arg(clip::help_arg)
        .arg(Argument<Str, flag, none, "name", "Name of the executable">{})
        .arg(Argument<std::vector<Str>, flag, none, "implicit-arg",
                      "Implicit arguments to pass to script">{})
        .arg(Argument<bool, flag, "D", "delete", "Delete executable shim">{})
        .arg(Argument<bool, flag, "l", "list", "List registered executables">{})
        .arg(Argument<Str, positional, none, "script", "Script to register">{});

constexpr auto comp =
    Subcommand<none, "completion", "Print the completion script for crun">{}
        .arg(clip::help_arg)
        .arg(Argument<Str, positional, none, "SHELL",
                      "Which shell to complete for">{});

constexpr auto bin = Subcommand<none, "bin", "Print the binary directory">{};

constexpr auto cli_parser =
    Command<none, "crun", "Compiles and Runs programs">{}
        .arg(Argument<bool, flag, "h", "help", "Prints help">{})
        .arg(run)
        .arg(build)
        .arg(reg)
        .arg(comp)
        .arg(bin);

using Args = typename decltype(cli_parser)::type;

void app(Args const &args);

int main(int argc, char *argv[]) {
  try {
    app(cli_parser.parse(argc, argv));
  } catch (std::runtime_error const &e) {
    std::println("Caught error: {}", e.what());
    return 2;
  } catch (Str const &s) {
    std::puts(s.c_str());
    return 1;
  }
}

void run_cmd(auto const &, std::size_t col);
void build_cmd(auto const &, std::size_t col);
void register_cmd(auto const &, std::size_t col);
void completions_cmd(auto const &, std::size_t col);
void bin_cmd(auto const &);

fs::path cache_dir();
fs::path config_dir();

namespace lg {
size_t level = 0;
template <typename... Args>
constexpr void error(std::format_string<Args...> fmt, Args &&...args) {
  if (level >= 0)
    std::println(stderr, "ERROR: {}",
                 std::format(fmt, std::forward<Args>(args)...));
}
template <typename... Args>
constexpr void warn(std::format_string<Args...> fmt, Args &&...args) {
  if (level >= 1)
    std::println(stderr, "WARNING: {}",
                 std::format(fmt, std::forward<Args>(args)...));
}
template <typename... Args>
constexpr void info(std::format_string<Args...> fmt, Args &&...args) {
  if (level >= 2)
    std::println(stderr, "INFO: {}",
                 std::format(fmt, std::forward<Args>(args)...));
}
} // namespace lg

void app(Args const &args) {
  auto &[help, ru, bu, re, co, bi] = args;
  // todo crossplatform?
  winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  if (help)
    std::println("{}", cli_parser.help({.cols = w.ws_col}));
  if (ru)
    run_cmd(*ru, w.ws_col);
  if (bu)
    build_cmd(*bu, w.ws_col);
  if (re)
    register_cmd(*re, w.ws_col);
  if (co)
    completions_cmd(*co, w.ws_col);
  if (bi)
    bin_cmd(*bi);
}

void run_cmd(auto const &args, std::size_t col) {
  auto &[help, script, verbosity, compdb, compdb_path] = args;
  if (help) {
    auto h = run.help({.cols = col});
    std::puts(h.c_str());
    return;
  }
  lg::level = verbosity;
}

constexpr std::string_view ninja_prologue = R"(
cxxflags = -std=c++23 -fmodules -O3 -Wall -flto=auto -march=native

rule cxx
  command = g++ $cxxflags $includes $depflags -x c++ -c $in -o $out

rule link
  command = g++ $cxxflags $in -o $out

)";

constexpr std::string_view regenerate_rule = R"(
rule regenerate_ninja
  command = crun build --configure $in
)";

constexpr std::string_view regenerate_bootstrap =
    "rule regenerate_ninja\n  command = " CRUN_ROOT "/bootstrap.sh $in\n";

void scan_deps(auto out, fs::path);
void output_rule(auto out, fs::path src, std::string_view include);

void build_cmd(auto const &args, std::size_t col) {
  auto &[help, verbosity, flags, compdb, compdb_path, rebuild, configure,
         script] = args;
  if (help) {
    auto h = build.help({.cols = col});
    std::puts(h.c_str());
    return;
  }
  lg::level = verbosity;
  if (script.empty()) {
    throw Str("Specify a script to build");
  }

  fs::path cache = cache_dir();
  auto script_path = fs::absolute(script);
  auto ninja_path = cache / script_path.relative_path();
  ninja_path.replace_extension(".ninja");
  lg::info("Writing ninjafile to path: {}", ninja_path.string());
  fs::create_directories(ninja_path.parent_path());
  auto ninjafile = std::ofstream(ninja_path);
  auto ninja = std::ostreambuf_iterator(ninjafile);

  std::ranges::copy(ninja_prologue, ninja);
  if (script != "crun") {
    std::ranges::copy(regenerate_rule, ninja);
  } else {
    std::ranges::copy(regenerate_bootstrap, ninja);
  }
  std::format_to(ninja, "build | {}: regenerate_ninja {}\n",
                 ninja_path.string(), script_path.string());

  scan_deps(ninja, fs::path(CRUN_ROOT) / "deps");

  output_rule(ninja, script_path, "-I.");

  auto output_name = fs::path(script).stem();

  auto obj = cache / script_path.relative_path();
  obj += ".o";

  auto out_name =
      (cache / script_path.parent_path().relative_path() / output_name)
          .string();
  std::format_to(ninja, "build {}: link {}\n", out_name, obj.string());
  std::format_to(ninja, "default {}\n", out_name);

  if (!configure) {
    auto s = ninja_path.string();
    sp::call({"ninja", "--quiet", "-f", s.c_str()});
  }
}

void scan_deps(auto out, fs::path depdir) {
  auto depfile = depdir / "deps.json";
  if (not fs::exists(depfile))
    throw Str(std::format("Expected deps.json at {}", depfile.string()));

  std::ifstream f(depfile);
  json deps = json::parse(f);

  // could probably be parallelized for large amounts of dependencies
  for (auto const &dep : deps) {
    auto sources = dep["sources"];
    auto includes = depdir / std::string(dep["include"]);
    auto include_str = "-I" + includes.string();
    for (auto const &src : sources) {
      output_rule(out, depdir / Str(src), include_str);
    }
  }
}

void output_rule(auto out, fs::path src, std::string_view include) {
  auto filepath = src.string();
  auto deps = sp::check_output({"g++", "-std=c++23", "-fmodules",
                                "-fdeps-format=p1689r5", "-fdeps-file=-",
                                "-xc++", "-MM", "-MF", "/dev/null",
                                filepath.c_str(), include.data()});
  auto depjson = json::parse(deps.buf);
  auto rules = depjson["rules"];
  if (rules.size() != 1)
    throw std::runtime_error("Schema of p1689r5 json is invalid");
  auto req = rules[0]["requires"];
  auto prov = rules[0]["provides"];
  std::format_to(out, "build {}.o",
                 (cache_dir() / src.relative_path()).string());
  if (prov.size() > 0) {
    std::format_to(out, " | ");
    for (auto const &p : prov) {
      std::format_to(out, "gcm.cache/{}.gcm ", Str(p["logical-name"]));
    }
  }
  std::format_to(out, " : cxx {} ", filepath);
  if (req.size() > 0) {
    std::format_to(out, " | ");
    for (auto const &r : req) {
      std::format_to(out, "gcm.cache/{}.gcm ", Str(r["logical-name"]));
    }
  }
  std::format_to(out, "\n  includes = {}", include);
  std::format_to(out, "\n  deps = gcc");
  auto basename = src.filename().string();
  std::format_to(out, "\n  depflags = -MD -Mno-modules -MQ {0}.o -MF {0}.d",
                 basename);
  std::format_to(out, "\n  depfile = {}.d", basename);
  std::format_to(out, "\n\n");
}

// Todo windows??
void register_cmd(auto const &args, std::size_t col) {
  auto &[help, name, impl_args, del, list, script_name] = args;
  if (help) {
    auto h = reg.help({.cols = col});
    std::puts(h.c_str());
    return;
  }
  auto bin = config_dir() / "bin";
  fs::create_directories(bin);
  auto script = fs::absolute(script_name);
  auto shim_name = name.empty() ? script.stem().string() : name;
  auto shim_path = bin / shim_name;
  auto shim = std::ofstream(shim_path);
  auto reg = std::ostreambuf_iterator(shim);

  constexpr std::string_view reg_script = "#!/usr/bin/sh\nset -euo pipefail\n";
  std::ranges::copy(reg_script, reg);
  auto binary = (cache_dir() / script.relative_path()).replace_extension("");
  if (shim_name != "crun") {
    std::format_to(reg,
                   "if [ ! -f {} ];then\n crun build --configure {};\nfi\n",
                   binary.string(), script.string());
  } else {
    // This has to be hardcoded otherwise it will continuously call itself
    auto root = script.parent_path();
    std::format_to(reg,
                   "if [ ! -f {0} ];then\n  {1}/bootstrap.sh "
                   "{1}/crun.cpp;\nfi\n",
                   binary.string(), root.string());
  }
  auto ninja = binary;
  ninja.replace_extension(".ninja");
  std::format_to(reg, "ninja --quiet -C {} -f {}\n", cache_dir().string(),
                 ninja.string());
  std::format_to(reg, "{} $@ ", binary.string());
  for (auto const &arg : impl_args) {
    std::format_to(reg, "{} ", arg);
  }
  *reg++ = '\n';
  using enum fs::perms;
  fs::permissions(shim_path, owner_exec, fs::perm_options::add);
}

void completions_cmd(auto const &args, std::size_t col) {
  auto &[help, shell] = args;
  if (help) {
    auto h = comp.help({.cols = col});
    std::puts(h.c_str());
    return;
  }
}

void bin_cmd(auto const &args) {
  auto bin = (config_dir() / "crun" / "bin").string();
  std::puts(bin.c_str());
}

fs::path cache_dir() {
  static std::optional<fs::path> cached;
  if (cached.has_value())
    return *cached;
  const char *cachepath = std::getenv("CRUN_CACHE");
  if (cachepath == nullptr || *cachepath == '\0') {
    const char *home = std::getenv("HOME");
    if (home == nullptr || *home == '\0')
      throw std::runtime_error("HOME is null for some reason");
    auto path = fs::path(home) / ".crun_cache";
    cached = std::move(path);
  }

  return *cached;
}

fs::path config_dir() {
  static std::optional<fs::path> cached;
  if (cached.has_value())
    return *cached;
  const char *xdg_config = std::getenv("XDG_CONFIG_HOME");
  const char *home = std::getenv("HOME");
  if (xdg_config == nullptr || *xdg_config == '\0') {
    if (home == nullptr || *home == '\0')
      throw std::runtime_error("HOME is null for some reason");
    cached = fs::path(home) / ".config" / "crun";
  } else {
    cached = fs::path(xdg_config) / "crun";
  }
  return *cached;
}

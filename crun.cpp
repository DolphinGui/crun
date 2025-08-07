#include <unistd.h>
#include <sys/ioctl.h>

import std;

import subprocess;
import nlohmann.json;
import clip;

using clip::Argument;
using clip::Command;
using clip::flag;
using clip::none;
using clip::positional;
using clip::Subcommand;

using Str = std::string;

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
        .arg(Argument<Str, flag, "o", "output", "Name of output file">{})
        .arg(Argument<Str, flag, none, "build-flags", "Flags to build with">{})
        .arg(compdb)
        .arg(compdb_path)
        .arg(Argument<bool, flag, none, "rebuild",
                      "Forcibly rebuilds everything">{})
        .arg(Argument<bool, flag, none, "configure",
                      "Only configures instead of building anything">{});

constexpr auto reg =
    Subcommand<"reg", "register", "Registers a command into PATH">{}
        .arg(clip::help_arg)
        .arg(Argument<Str, flag, none, "name", "Name of the executable">{})
        .arg(Argument<std::vector<Str>, flag, none, "implicit-arg",
                      "Implicit arguments to pass to script">{})
        .arg(Argument<bool, flag, "D", "delete", "Delete executable shim">{})
        .arg(
            Argument<bool, flag, "l", "list", "List registered executables">{});

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
  }
}

void run_cmd(auto const &, int col);
void build_cmd(auto const &, int col);
void register_cmd(auto const &, int col);
void completions_cmd(auto const &, int col);
void bin_cmd(auto const &, int col);

void app(Args const &args) {
  auto &[help, ru, bu, re, co, bi] = args;
  // todo crossplatform?
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  if (help)
    std::println("Help asked for!");
  if (ru)
    run_cmd(*ru, w.ws_col);
  if (bu)
    build_cmd(*bu, w.ws_col);
  if (re)
    register_cmd(*re, w.ws_col);
  if (co)
    completions_cmd(*co, w.ws_col);
  if (bi)
    bin_cmd(*bi, w.ws_col);
}
void run_cmd(auto const &args, int col) {
  auto &[help, script, verbosity, compdb, compdb_path] = args;
  if (help) {
    auto h = run.help({.cols = col});
    std::puts(h.c_str());
  }
  if (verbosity > 0)
    std::println("Verbosity level at {}", verbosity);
}
void build_cmd(auto const &args, int col) {
  auto &[help, verbosity, output, flags, compdb, compdb_path, rebuild,
         configure] = args;
  if (help)
    std::println("Build help asked for");
}
void register_cmd(auto const &args, int col) {
  auto &[help, name, impl_args, del, list] = args;
  if (help)
    std::println("Register help asked for");
}
void completions_cmd(auto const &args, int col) {
  auto &[help, shell] = args;
  if (help)
    std::println("Completion help asked for");
}
void bin_cmd(auto const &args, int col) {}

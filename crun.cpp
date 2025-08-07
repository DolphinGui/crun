#include <unistd.h>

import std;

import subprocess;
import nlohmann.json;
import clip;

using clip::Argument;
using clip::flag;
using clip::none;
using clip::Parser;
using clip::positional;

namespace sp = subprocess;
using json = nlohmann::json;

constexpr inline auto run_parse =
    Parser<none, "run", "Run program", true>{}
        .arg(Argument<bool, flag, "h", "help", "Prints help">{})
        .arg(Argument<std::string, positional, none, "script",
                      "Script to run">{});

constexpr inline auto cli_parser =
    Parser<none, "crun", "Compiles and Runs programs">{}
        .arg(Argument<bool, flag, "h", "help", "Prints help">{})
        .arg(run_parse);

using Args = typename decltype(cli_parser)::type;

void app(Args const &args);

int main(int argc, char *argv[]) {
  try {
    app(cli_parser.parse(argc, argv));
  } catch (std::runtime_error const &e) {
    std::println("Caught error: {}", e.what());
  }
}

void app(Args const &args) {
  auto &[help, run] = args;
  if (help)
    std::println("Help asked for!");
}

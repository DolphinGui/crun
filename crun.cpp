module;

#include <stdio.h>
#include <unistd.h>

export module app;

import std;

import subprocess;
import nlohmann.json;
import clip;
import appmain;

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

export constexpr inline auto cli_parser =
    Parser<none, "crun", "Compiles and Runs programs">{}
        .arg(Argument<bool, flag, "h", "help", "Prints help">{})
        .arg(run_parse);

using Args = typename decltype(cli_parser)::type;

export void app(Args args) {}

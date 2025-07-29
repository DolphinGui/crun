# crun

This is a build system built to learn about modules support in C++. Yes, it is currently
primarily written in bash. Yes, it is absolutely horrid.

## Purpose

crun is built to be similar to (cargo-script)[https://github.com/danielkeep/cargo-script],
which allows for rust files to be used like scripting files. Currently only the bootstrap
script works, but eventually (probably once clangd fixes module support), I'll rewrite it
in modern C++.

It uses GCC to scan module dependencies, then uses a prebuilt module-map (not an actual module-map,
but just a json file) in order to build the dependencies ahead of time.

## Features

`import std;`! Import dependencies and just see them work! Actually somehow less painful than a
lot of C++ package management despite being as robust as duct tape and cope!

## Limitations

Dependencies that depend on dependencies aren't currently supported. Also, multiple cpp files
currently aren't supported. Using header files instead of modules doesn't really work, but that's
a non-goal. It also generates compile_commands.json, but clangd doesn't really know how to deal
with modules.



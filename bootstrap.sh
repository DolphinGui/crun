#!/usr/bin/bash

set -euo pipefail

mkdir -p $CRUN_CACHE/bin

jaql=$(realpath $(dirname $0)/../jaql.py)
src=$(realpath $(dirname $0))

dep_paths="$CRUN_DEPS:$src/deps"

script=$1
exe=$(basename $script .cpp)
includes=""
IFS=':'
for dep in $dep_paths; do
  for include in $(cat $dep/deps.json | $jaql "*(?.'header').'include'" --seperator :); do
    includes="-I$dep/$include $includes"
  done
done

pushd $CRUN_CACHE

# module dependencies of script
dep_mods=$(g++ -std=c++23 -fmodules -fdeps-format=p1689r5 -fdeps-file=- \
  -fdeps-target="$script.o" -MM -MG -MF /dev/null "$src/$script" | $jaql "(.'rules'*.'requires'*.'logical-name')++")

dep_gcms=""
IFS=' '
for dep in $dep_mods; do
  dep_gcms="gcm.cache/$dep.gcm $dep_gcms"
done

cat <<EOF > $exe.ninja

cxxflags = -std=c++23 -fmodules -O3 -flto=auto -Wall -march=native

rule cxx
  command = g++ \$cxxflags \$includes -fsearch-include-path -x c++ -c \$in -o \$out

rule gen_std
  command = g++ \$cxxflags \$includes -fsearch-include-path -c bits/std.cc -o \$out

build std.o | gcm.cache/std.gcm: gen_std

rule link
  command = g++ \$cxxflags \$in -o \$out
EOF

objects=""
IFS=' '
for dep in $dep_mods; do
  if [ $dep = "std" ]; then
    continue
  fi
  IFS=':';
  finished=""
  for dep_path in $dep_paths; do
    info=$(cat $dep_path/deps.json | $jaql "*?='$dep'.'module_name'" || echo "")
    if [ "$info" = "" ]; then continue; fi;
    include=$($jaql ".'include'" "$info")
    interface=$($jaql ".'interface_file'" "$info")
    printf "build $dep.o | gcm.cache/$dep.gcm: cxx $dep_path/$interface\n  includes = -I $dep_path/$include\n" >> $exe.ninja
    objects="$dep.o $objects"
    finished="done"
    break;
  done
  if [ finished = "" ]; then exit 1; fi;
done
printf "build $script.o: cxx $src/$script | $dep_gcms\n  includes = $includes\n" >> $exe.ninja
printf "build bin/$exe: link $script.o std.o $objects\n" >> $exe.ninja
ninja -f $exe.ninja
ninja -f $exe.ninja -t compdb > compile_commands.json
popd
mv $CRUN_CACHE/compile_commands.json .
shift 1
$CRUN_CACHE/bin/$exe $*

#!/usr/bin/bash

set -euo pipefail

jaql=$(realpath $(dirname $0)/../jaql.py)
src=$(realpath $(dirname $0))

global_deps="${CRUN_DEPS:-}"
cache_dir="${CRUN_CACHE:-$HOME/.crun_cache}"
dep_paths="$global_deps:$src/deps"


script=$(realpath $1)
exe=$(basename $script .cpp)
this_script=$(realpath $0)
REGENERATE="${REGENERATE:-0}"
mkdir -p "$cache_dir"
pushd $cache_dir > /dev/null

if [ ! -f "$cache_dir/$exe.ninja" ] || [ ! "$REGENERATE" = 0 ]; then
if [ ! "$REGENERATE" = 0 ]; then
  ninjafile="/dev/stdout"
else
  ninjafile="$exe.ninja"
fi

includes=""
IFS=':'
for dep in $dep_paths; do
  for include in $(cat $dep/deps.json | $jaql "*(?.'header').'include'" --seperator :); do
    includes="-I$dep/$include $includes"
  done
done

cat > $ninjafile  <<EOF
#shellgen
cxxflags = -std=c++23 -fmodules -O3 -Wall -flto=auto -march=native

rule cxx
  command = g++ \$cxxflags \$includes \$depflags -x c++ -c \$in -o \$out

rule link
  command = g++ \$cxxflags \$in -o \$out

rule regenerate_ninja
  command = env REGENERATE=1 \$in > \$out

build $exe.ninja : regenerate_ninja $this_script $script
build regenerate : phony $exe.ninja

EOF

objects=""
files_done=""
output_rule()
{
local target_file=$1
local obj=$(basename $target_file)
local includes=$2
local dep_info=$(g++ -std=c++23 -fmodules -fdeps-format=p1689r5 -fdeps-file=- \
  -fdeps-target="$obj.o" -xc++ -MM -MF /dev/null "$target_file" $includes)
local dep_mods=$(echo "$dep_info" | $jaql "(.'rules'*.'requires'*.'logical-name')++" || echo "")
local output_gcm=$(echo "$dep_info" | $jaql "(.'rules'*.'provides'*.'logical-name')++" | \
              sed -nE "s/(:?\s|^)(\S+)/gcm.cache\/\2.gcm /pg" || echo "")
files_done="$1 $files_done"
IFS=' '
for dep in $dep_mods; do
  if echo "$dep" | grep -q "std"; then continue; fi;
  IFS=':'
  for dep_path in $dep_paths; do
    local info=$(cat $dep_path/deps.json | $jaql "(*?='$dep'.'module_name')!^" || echo "")
    if [ "$info" = "" ]; then continue; fi;
    local files=$(echo "$info" | $jaql ".'sources'" --seperator=':')
    local include=$(echo "$info" | $jaql ".'include'")
    IFS=':'
    for file in $files; do
      if ! echo "$files_done" | grep -q "$file"; then
        output_rule "$dep_path/$file" "-I$dep_path/$include"
      fi
    done
    break;
  done
done
local dep_gcms=$(echo "$dep_mods" | sed -E "s/(:?\s|^)(\S+)/gcm.cache\/\2.gcm /g")
# do not output duplicate rules
if grep -q "build $obj.o" $exe.ninja ; then return; fi
printf "build $obj.o | $output_gcm: cxx $target_file | $dep_gcms\n  includes = $includes\n" >> $ninjafile
if [ ! -z "$includes" ]; then
  depfile="$obj.d"
  printf "  depflags = -MD -Mno-modules -MQ $obj.o -MF $depfile\n" >> $ninjafile
  printf "  deps = gcc\n  depfile = $depfile\n" >> $ninjafile
fi
target=$(basename $target_file)
objects="$target.o $objects"
}

stdjson=$(gcc -print-file-name=libstdc++.modules.json)
stdmod=$(cat $stdjson | $jaql ".'modules'*" --seperator=';')
IFS=';'
for mod in $stdmod; do
  modname=$(echo "$mod" | $jaql ".'logical-name'")
  basepath=$(dirname $stdjson)
  srcpath=$(echo "$mod" | $jaql ".'source-path'")
  srcfile=$(basename $srcpath)
  path=$(realpath $basepath/$srcpath)
  dep=""
  if [ "$modname" = "std.compat" ]; then
    dep="gcm.cache/std.gcm"
  fi
  echo "build $srcfile.o | gcm.cache/$modname.gcm: cxx $path | $dep" >> $ninjafile
done

output_rule "$script" ""

printf "build bin/$exe: link $objects std.cc.o\n" >> $ninjafile
printf "default bin/$exe\n" >> $ninjafile 

fi

if [ "$REGENERATE" = 0 ]; then
ninja -f $exe.ninja --quiet
ninja -f $exe.ninja -t compdb cxx > compile_commands.json
popd > /dev/null
mv -f $cache_dir/compile_commands.json . || echo ""
shift 1
$cache_dir/bin/$exe $*
fi

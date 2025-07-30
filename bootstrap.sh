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

cat > $exe.ninja <<EOF

cxxflags = -std=c++23 -fmodules -O2 -Wall -flto=auto -march=native

rule cxx
  command = g++ \$cxxflags \$includes -x c++ -c \$in -o \$out

rule link
  command = g++ \$cxxflags \$in -o \$out
EOF

objects=""
files_done=""
output_rule()
{
local target_file=$1
local includes=$2
local dep_info=$(g++ -std=c++23 -fmodules -fdeps-format=p1689r5 -fdeps-file=- \
  -fdeps-target="$target_file.o" -xc++ -MM -MG -MF /dev/null "$target_file")
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
    local files=$(echo "$info" | $jaql ".'file'" --seperator=':')
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
local obj=$(basename $target_file)
if grep -q "build $obj.o" $exe.ninja; then return; fi
printf "build $obj.o | $output_gcm: cxx $target_file | $dep_gcms\n  includes = $includes\n" >> $exe.ninja
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
  echo "build $srcfile.o | gcm.cache/$modname.gcm: cxx $path" >> $exe.ninja
done

output_rule $src/$script ""

printf "build bin/$exe: link std.cc.o $objects\n" >> $exe.ninja
printf "default bin/$exe\n" >> $exe.ninja
ninja -f $exe.ninja
ninja -f $exe.ninja -t compdb > compile_commands.json
popd
mv $CRUN_CACHE/compile_commands.json .
shift 1
$CRUN_CACHE/bin/$exe $*


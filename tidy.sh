#!/bin/bash

. ./prebuild.sh "$1"
find ../src/ -iname '*.h' -o -iname '*.cpp' | xargs clang-tidy-18 -p="build" --config-file="../.clang-tidy" -extra-arg=-std=c++20
#!/bin/bash

. ./prebuild.sh "$1"
find ../ -path ./build -prune -o -iname '*.hpp' -o -iname '*.cpp' | xargs clang-tidy-18 -p="build" --config-file="../.clang-tidy" -extra-arg=-std=c++20
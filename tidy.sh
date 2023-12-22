#!/bin/sh

mkdir build
cd ./build
cmake --build . --target clean
cmake -G "Ninja" .. -D CMAKE_BUILD_TYPE=$1 -D CMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy-18 -p="build" --config-file="../.clang-tidy" -extra-arg=-std=c++20 ../kd_tree.hpp
clang-tidy-18 -p="build" --config-file="../.clang-tidy" -extra-arg=-std=c++20 ../kd_tree_stack_optimized.hpp
clang-tidy-18 -p="build" --config-file="../.clang-tidy" -extra-arg=-std=c++20 ../tests/test.cpp
ninja


mkdir build
cd ./build
cmake --build . --target clean
cmake -G "Ninja" .. -D CMAKE_BUILD_TYPE=$1 -D CMAKE_EXPORT_COMPILE_COMMANDS=ON
find ../src/ -iname '*.h' -o -iname '*.cpp' | xargs clang-tidy-18 -p="build" --config-file="../.clang-tidy" -extra-arg=-std=c++20
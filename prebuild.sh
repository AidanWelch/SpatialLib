#!/bin/bash

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
mkdir build
cd ./build
cmake --build . --target clean
cmake -G "Ninja" .. -D CMAKE_BUILD_TYPE=${1:-Release} -D CMAKE_EXPORT_COMPILE_COMMANDS=ON
#!/bin/sh

mkdir build
cd ./build
cmake --build . --target clean
cmake -G "Ninja" .. -D CMAKE_BUILD_TYPE=$1 -D CMAKE_EXPORT_COMPILE_COMMANDS=ON
ninja
#!/bin/bash

find . -path ./build -prune -iname '*.h' -o -iname '*.cpp' | xargs clang-format-18 -style=file -i
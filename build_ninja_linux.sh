#!/bin/sh
#change current directory to this file
SCRIPT_PATH="$(dirname "$0")"
cd "$SCRIPT_PATH" || exit 1

build_dir=_build/ninja-linux

cmake \
    -G "Ninja" \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -B $build_dir \
    .

cmake --build $build_dir
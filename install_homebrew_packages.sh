#!/bin/sh

brew install --cask vulkan-sdk

brew install \
    cmake \
    ninja \
    pkg-config \
    curl \
    zlib \
    assimp \
    glfw \
    sdl2 \
    sdl2_mixer \
    || exit 1

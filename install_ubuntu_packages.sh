#!/bin/sh

apt install -y \
    build-essential \
    cmake \
    clang \
    clang-tools \
    ninja-build \
    pkg-config \
    curl \
	libglu1-mesa-dev \
    glslang-tools \
    glslc \
    libvulkan-dev \
    vulkan-tools \
    vulkan-validationlayers \
    zlib1g-dev \
    libassimp-dev \
    libglfw3-dev \
    libsdl2-dev \
    libsdl2-mixer-dev \
    || exit 1
#----------------------------

check_pkg() {
    local pkg_name=$1
    local apt_package=$2

    if pkg-config --modversion "$pkg_name" >/dev/null 2>&1; then
        echo "✓ $pkg_name found: $(pkg-config --modversion $pkg_name)"
        return 0
    else
        echo "✗ $pkg_name NOT found"
        echo "  Install with: sudo apt install $apt_package"
        return 1
    fi
}

check_cmd() {
    local cmd_name=$1
    local apt_package=$2

    if command -v "$cmd_name" >/dev/null 2>&1; then
        echo "✓ $cmd_name found: $(command -v $cmd_name)"
        return 0
    else
        echo "✗ $cmd_name NOT found"
        echo "  Install with: sudo apt install $apt_package"
        return 1
    fi
}

missing=0
check_pkg glfw3 libglfw3-dev            || missing=1
check_pkg assimp libassimp-dev          || missing=1
check_pkg vulkan libvulkan-dev          || missing=1
check_pkg zlib zlib1g-dev               || missing=1
check_pkg sdl2 libsdl2-dev              || missing=1
check_pkg SDL2_mixer libsdl2-mixer-dev  || missing=1
check_cmd glslc glslang-tools           || missing=1

if [ $missing -ne 0 ]; then
    echo "\nSome packages are missing. Install them and re-run."
    exit 1
fi


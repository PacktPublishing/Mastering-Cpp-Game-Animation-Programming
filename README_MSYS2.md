# Compiling 'Mastering C++ Game Animation Programming' with MSYS2

The example code for the book can be compiled with *GCC* or *Clang* from *MSYS2*.

The *Vulkan SDK* is **optional** when using *MSYS2*: *GLM* will be installed and *VMA* will be downloaded if the *Vulkan SDK* could not be found.
Also, a separate installation of the *Open Asset Import Library* ('Assimp') and *SDL2* is **not** needed.

## Download and Install

Download the latest installer package from the homepage of the *MSYS2* project:
[https://www.msys2.org](https://www.msys2.org)

After installing MSYS2, upgrade the installation by using the following command:
```
pacman -Syu
```

The terminal window will close after the upgrade, you need to open a new *MSYS2* terminal (**MINGW** or **UCRT**).

## Packages to install

The list of packages is mostly identical to the Linux version.

We need the following packages for general compilation:
* git
* cmake
* ninja
* gcc (or clang)
* glm
* glfw
* assimp
* zlib

If you want to compile the Vulkan examples, the following two packages are needed:
* vulkan-devel
* glslang

For Chapter 5 and later, *YAML-CPP* is needed:
* yaml-cpp

For Chater 14, *SDL* is needed:
* SDL2
* SDL2\_mixer


### Installing for MINGW

Use the following command to install all required packages for **MINGW** builds:
```
pacman -S git mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja\
  mingw-w64-x86_64-gcc mingw-w64-x86_64-glm mingw-w64-x86_64-glfw\
  mingw-w64-x86_64-assimp mingw-w64-x86_64-zlib\
  mingw-w64-x86_64-vulkan-devel mingw-w64-x86_64-glslang\
  mingw-w64-x86_64-yaml-cpp mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer
```

To use *Clang* instead of *GCC*, you need to install the **clang** package:

```
pacman -S mingw-w64-x86_64-clang
```

You must also set the following two environment variables **before** running *CMake*:
```
export CC=/mingw64/bin/clang.exe
export CXX=/mingw64/bin/clang++.exe
```

### Installing for UCRT

Use this commandline to install all required packages for UCRT builds:
```
pacman -S git mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja\
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-glm mingw-w64-ucrt-x86_64-glfw\
  mingw-w64-ucrt-x86_64-assimp mingw-w64-ucrt-x86_64-zlib\
  mingw-w64-ucrt-x86_64-vulkan-devel mingw-w64-ucrt-x86_64-glslang\
  mingw-w64-ucrt-x86_64-yaml-cpp mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_mixer
```


To use *Clang* instead of *GCC*, you need to install the **clang** package:

```
pacman -S mingw-w64-ucrt-x86_64-clang
```

You must also set the following two environment variables **before** running *CMake*:
```
export CC=/ucrt64/bin/clang.exe
export CXX=/ucrt64/bin/clang++.exe
```

## Compiling and running a single example

To compile and run the code for a single example, use the following steps. Replace chapter number and example folder name accordingly:
```
cd chapter14
cd 01_opengl_ideas
mkdir build
cd build
cmake -G Ninja ..
ninja
./Main
```

**Note: Starting the *Main.exe* only works from the MSYS2 terminal!**

## Building all chapters at once

The global *CMakeLists.txt* file also supports building all chapters at once. In the top level folder of the example code, execute the following commands:

```
mkdir build
cd build
cmake -G Ninja ..
ninja
```

A folder named **bin** will bee created in the top level folder, containing a folder for each chapter and the two example folders in each chapter's folder.

**Note: The Vulkan examples will be only compiled if the *vulkaninfo* executable has been found!**

**Note: Starting any *Main.exe* inside the *bin* folder only works from the MSYS2 terminal!**

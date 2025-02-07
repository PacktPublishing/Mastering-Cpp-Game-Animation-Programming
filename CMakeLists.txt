## CMakeLists.txt to build all examples
##
## Per-chapter build:
## Use the CMakeLists.txt files in each example's folders to build only that example

cmake_minimum_required(VERSION 3.19)

# Prevent in-source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD TRUE)
set(CMAKE_DISABLE_SOURCE_CHANGES TRUE)

# Check if the project is being built in the source directory
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Please create a build directory and build there. You might have to remove 'CMakeCache.txt' and 'CMakeFiles'!")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Mastering-Cpp-Game-Animations)

include(ExternalProject)
macro(BuildOpenGLProject ChapterNum ExampleName)
  ExternalProject_Add(
    Chapter${ChapterNum}-OpenGL
    PREFIX ${CMAKE_BINARY_DIR}/build-files
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}
    BINARY_DIR ${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build

    INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/Main${CMAKE_EXECUTABLE_SUFFIX}" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/imgui.ini" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/assets/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/assets/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/shader/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/shader/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/textures/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/textures/"
    # show output of Ninja build - implicitely serializes builds
    USES_TERMINAL_BUILD true
  )
endmacro()

macro(BuildOpenGLProjectWithConfig ChapterNum ExampleName)
  ExternalProject_Add(
    Chapter${ChapterNum}-OpenGL-Config
    PREFIX ${CMAKE_BINARY_DIR}/build-files
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}
    BINARY_DIR ${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build

    INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/Main${CMAKE_EXECUTABLE_SUFFIX}" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/imgui.ini" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/assets/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/assets/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/shader/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/shader/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/textures/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/textures/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/config/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/config/"
    USES_TERMINAL_BUILD true
  )
endmacro()

if (WIN32)
  macro(BuildOpenGLProjectWithConfigAndSDL2 ChapterNum ExampleName)
    ExternalProject_Add(
      Chapter${ChapterNum}-OpenGL-Config-SDL2
      PREFIX ${CMAKE_BINARY_DIR}/build-files
      SOURCE_DIR ${PROJECT_SOURCE_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}
      BINARY_DIR ${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build

      INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}"
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/Main${CMAKE_EXECUTABLE_SUFFIX}" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/"
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/imgui.ini" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/"
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/SDL2.dll" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/"
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/SDL2_mixer.dll" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/assets/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/assets/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/shader/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/shader/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/textures/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/textures/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/01_opengl_${ExampleName}-build/$<CONFIG>/config/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/01_opengl_${ExampleName}/config/"
      USES_TERMINAL_BUILD true
    )
  endmacro()
endif()

macro(BuildVulkanProject ChapterNum ExampleName)
  ExternalProject_Add(
    Chapter${ChapterNum}-Vulkan
    PREFIX ${CMAKE_BINARY_DIR}/build-files
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}
    BINARY_DIR ${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build

    INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/Main${CMAKE_EXECUTABLE_SUFFIX}" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/imgui.ini" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/assets/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/assets/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/shader/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/shader/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/textures/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/textures/"
    USES_TERMINAL_BUILD true
  )
endmacro()

macro(BuildVulkanProjectWithConfig ChapterNum ExampleName)
  ExternalProject_Add(
    Chapter${ChapterNum}-Vulkan-Config
    PREFIX ${CMAKE_BINARY_DIR}/build-files
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}
    BINARY_DIR ${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build

    INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/Main${CMAKE_EXECUTABLE_SUFFIX}" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/imgui.ini" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/assets/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/assets/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/shader/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/shader/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/textures/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/textures/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/config/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/config/"
    USES_TERMINAL_BUILD true
  )
endmacro()

if (WIN32)
  macro(BuildVulkanProjectWithConfigAndSDL2 ChapterNum ExampleName)
    ExternalProject_Add(
      Chapter${ChapterNum}-Vulkan-Config-SDL2
      PREFIX ${CMAKE_BINARY_DIR}/build-files
      SOURCE_DIR ${PROJECT_SOURCE_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}
      BINARY_DIR ${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build

      INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}"
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/Main${CMAKE_EXECUTABLE_SUFFIX}" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/"
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/imgui.ini" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/"
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/SDL2.dll" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/"
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/SDL2_mixer.dll" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/assets/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/assets/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/shader/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/shader/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/textures/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/textures/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/chapter${ChapterNum}/02_vulkan_${ExampleName}-build/$<CONFIG>/config/" "${PROJECT_SOURCE_DIR}/bin/chapter${ChapterNum}/02_vulkan_${ExampleName}/config/"
      USES_TERMINAL_BUILD true
    )
  endmacro()
endif()


# OpenGL examples
BuildOpenGLProject(01 assimp)
BuildOpenGLProject(02 computeshader)
BuildOpenGLProject(03 selection)
BuildOpenGLProject(04 edit_view_mode)
BuildOpenGLProjectWithConfig(05 load_save)
BuildOpenGLProjectWithConfig(06 cameras)
BuildOpenGLProjectWithConfig(07 animations)
BuildOpenGLProjectWithConfig(08 collisions)
BuildOpenGLProjectWithConfig(09 behavior)
BuildOpenGLProjectWithConfig(10 morphanim)
BuildOpenGLProjectWithConfig(11 level)
BuildOpenGLProjectWithConfig(12 adv_collision)
BuildOpenGLProjectWithConfig(13 navigation)
if (WIN32)
  BuildOpenGLProjectWithConfigAndSDL2(14 ideas)
else()
  BuildOpenGLProjectWithConfig(14 ideas)
endif()

# Check if Vulkan SDK is installed
find_file(results NAMES vulkaninfo${CMAKE_EXECUTABLE_SUFFIX})

# If yes, build Vulkan examples too
if (results)
  BuildVulkanProject(01 assimp)
  BuildVulkanProject(02 computeshader)
  BuildVulkanProject(03 selection)
  BuildVulkanProject(04 edit_view_mode)
  BuildVulkanProjectWithConfig(05 load_save)
  BuildVulkanProjectWithConfig(06 cameras)
  BuildVulkanProjectWithConfig(07 animations)
  BuildVulkanProjectWithConfig(08 collisions)
  BuildVulkanProjectWithConfig(09 behavior)
  BuildVulkanProjectWithConfig(10 morphanim)
  BuildVulkanProjectWithConfig(11 level)
  BuildVulkanProjectWithConfig(12 adv_collision)
  BuildVulkanProjectWithConfig(13 navigation)
  if (WIN32)
    BuildVulkanProjectWithConfigAndSDL2(14 ideas)
  else()
    BuildVulkanProjectWithConfig(14 ideas)
  endif()
endif()

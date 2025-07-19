cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Main)

# use custom file to find libraries
if(WIN32)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
endif()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_vma")

# we need the find scripts for older Linux distros
if(UNIX)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_unix")
endif()

# optimize for speed on Clang or GCC
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "-O3")
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  message("-- Using ccache")
  set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif(CCACHE_FOUND)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# get ImGui, no build needed (only the files)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG        v1.91.2
  GIT_SHALLOW 1
)

FetchContent_MakeAvailable(imgui)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
  add_subdirectory(${imgui_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()

# get ImGuiFileDialog, also files only
FetchContent_Declare(
  filedialog
  GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog
  GIT_TAG        v0.6.7
  GIT_SHALLOW 1
)

# ugly hack to have ImGui available when the file dialog source getch fetched
get_property(current_compile_options DIRECTORY PROPERTY COMPILE_OPTIONS)
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS -I ${imgui_SOURCE_DIR})
FetchContent_MakeAvailable(filedialog)
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS ${current_compile_options})
unset(current_compile_options)

FetchContent_GetProperties(filedialog)
if(NOT imgui_POPULATED)
  FetchContent_Populate(filedialog)
  add_subdirectory(${filedialog_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()

# get STB image reader
FetchContent_Declare(
  stbi
  GIT_REPOSITORY https://github.com/nothings/stb
  GIT_TAG        master
  GIT_SHALLOW 1
)

FetchContent_MakeAvailable(stbi)

FetchContent_GetProperties(stbi)
if(NOT stbi_POPULATED)
  FetchContent_Populate(stbi)
  add_subdirectory(${stbi_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()

# imnodes
FetchContent_Declare(
  imnodes
  GIT_REPOSITORY https://github.com/Nelarius/imnodes
  GIT_TAG v0.5
  GIT_SHALLOW 1
)

FetchContent_MakeAvailable(imnodes)

FetchContent_GetProperties(imnodes)
if(NOT imnodes_POPULATED)
  FetchContent_Populate(imnodes)
  add_subdirectory(${imnodes_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()

# get vk-bootstrap
include(FetchContent)
FetchContent_Declare(
  vk_bootstrap
  GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
  # with Ubuntu, v1.3.296 and newer broke compile in PFN_vkCmdSetDepthClampRangeEXT
  GIT_TAG        v1.3.295
  GIT_SHALLOW 1
)

FetchContent_MakeAvailable(vk_bootstrap)

FetchContent_GetProperties(vk_bootstrap)
if(NOT vk_bootstrap_POPULATED)
  FetchContent_Populate(vk_bootstrap)
  add_subdirectory(${vk_bootstrap_SOURCE_DIR} ${vk_bootstrap_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# get VMA
# Skip downloading if Vulkan SDK has been found
if (NOT DEFINED ENV{VK_SDK_PATH})
  include(FetchContent)
  FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG        v3.1.0
    GIT_SHALLOW 1
  )

  FetchContent_MakeAvailable(vma)

  FetchContent_GetProperties(vma)
  if(NOT vma_POPULATED)
    FetchContent_Populate(vma)
    add_subdirectory(${vma_SOURCE_DIR} EXCLUDE_FROM_ALL)
  endif()

  # set variable that will be used later
  set(VMA_INCLUDE_DIR ${vma_SOURCE_DIR}/vma)
endif()

# get YAML-CPP, GLFW3 and GLM via FetchContent on Windows systems
if(WIN32 AND NOT MINGW)
  # yaml-cpp
  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp
    GIT_TAG 0.8.0
    GIT_SHALLOW 1
  )

  FetchContent_MakeAvailable(yaml-cpp)

  FetchContent_GetProperties(yaml-cpp)
  if(NOT yaml-cpp_POPULATED)
    FetchContent_Populate(yaml-cpp)
    add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()

  # get and build GLFW3
  FetchContent_Declare(
    glfw3
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG        3.4
    GIT_SHALLOW 1
  )

  FetchContent_MakeAvailable(glfw3)

  FetchContent_GetProperties(glfw3)
  if(NOT glfw3_POPULATED)
    FetchContent_Populate(glfw3)
    add_subdirectory(${glfw3_SOURCE_DIR} ${glfw3_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()

  # set variables that will be used later
  set(GLFW3_INCLUDE_DIR ${glfw3_SOURCE_DIR}/include)
  set(GLFW3_LIBRARY "${glfw3_BINARY_DIR}/src/$<CONFIGURATION>/glfw3.lib")

  # Skip downloading if Vulkan SDK has been found
  if (NOT DEFINED ENV{VK_SDK_PATH})
    # get GLM
    FetchContent_Declare(
      glm
      GIT_REPOSITORY https://github.com/g-truc/glm
      GIT_TAG        1.0.1
      GIT_SHALLOW 1
    )

    # GLM is header only, but needs to be populated for the download
    FetchContent_MakeAvailable(glm)

    FetchContent_GetProperties(glm)
    if(NOT glm_POPULATED)
      FetchContent_Populate(glm)
      add_subdirectory(${glm_SOURCE_DIR} EXCLUDE_FROM_ALL)
    endif()

    # set variables that will be used later
    set(GLM_INCLUDE_DIR ${glm_SOURCE_DIR})
  endif()
endif()

file(GLOB SOURCES
  Main.cpp
  window/*.cpp
  tools/*.cpp
  vulkan/*.cpp
  model/*.cpp
  octree/*.cpp
  graphnodes/*.cpp
  ${imgui_SOURCE_DIR}/*.cpp
  # .cpp files for backends
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  ${filedialog_SOURCE_DIR}/*.cpp
  ${vk_bootstrap_SOURCE_DIR}/*.cpp
  ${imnodes_SOURCE_DIR}/imnodes.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC window tools vulkan model octree graphnodes)

# non-standard include dirs
target_include_directories(${PROJECT_NAME} PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends
  ${filedialog_SOURCE_DIR} ${stbi_SOURCE_DIR} ${yaml-cpp_SOURCE_DIR} ${imnodes_SOURCE_DIR}
  ${vk_bootstrap_SOURCE_DIR}/src ${vma_SOURCE_DIR}/include)

find_package(Vulkan REQUIRED)

find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(vma REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

include_directories(${GLFW3_INCLUDE_DIR} ${GLM_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIR} ${YAML_CPP_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIR} ${VMA_SOURCE_DIR}/vma)


# compile shaders
file(GLOB GLSL_SOURCE_FILES
  shader/*.frag
  shader/*.vert
  shader/*.comp
)

if(Vulkan_GLSLC_EXECUTABLE)
  message("Using glslc to compile shaders")
  foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_SOURCE_DIR}/shader/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${Vulkan_GLSLC_EXECUTABLE} -o ${SPIRV} ${GLSL}
      DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  endforeach(GLSL)
elseif (Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
  message("Using glslangValidator to compile shaders")
  foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_SOURCE_DIR}/shader/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V -o ${SPIRV} ${GLSL}
      DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  endforeach(GLSL)
endif()

add_custom_target(
  Shaders
  DEPENDS ${SPIRV_BINARY_FILES}
)
add_dependencies(${PROJECT_NAME} Shaders)

add_custom_command(TARGET Shaders POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/shader"
  "$<TARGET_PROPERTY:${PROJECT_NAME},BINARY_DIR>/$<CONFIGURATION>/shader"
)

# copy textures
file(GLOB TEX_SOURCE_FILES
  textures/*
)

add_custom_target(
  Textures
  DEPENDS ${TEX_SOURCE_FILES}
)
add_dependencies(${PROJECT_NAME} Textures)

add_custom_command(TARGET Textures POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/textures"
  "$<TARGET_PROPERTY:${PROJECT_NAME},BINARY_DIR>/$<CONFIGURATION>/textures"
)

# copy assets
file(GLOB ASSET_FILES
  assets/*
)

add_custom_target(
  Assets
  DEPENDS ${ASSET_FILES}
)

add_dependencies(${PROJECT_NAME} Assets)

add_custom_command(TARGET Assets POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/assets"
  "$<TARGET_PROPERTY:${PROJECT_NAME},BINARY_DIR>/$<CONFIGURATION>/assets"
)

# copy config files
file(GLOB CONFIG_FILE
  config/*
)

add_custom_target(
  ConfigFile
  DEPENDS ${CONFIG_FILE}
)

add_dependencies(${PROJECT_NAME} ConfigFile)

add_custom_command(TARGET ConfigFile POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/config"
  "$<TARGET_PROPERTY:${PROJECT_NAME},BINARY_DIR>/$<CONFIGURATION>/config"
)

# copy imgui.ini
file(GLOB IMGUI_INI imgui.ini)

add_custom_target(
  ImGuiIniFile
  DEPENDS ${IMGUI_INI}
)

add_dependencies(${PROJECT_NAME} ImGuiIniFile)

add_custom_command(TARGET ImGuiIniFile POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/imgui.ini"
  "$<TARGET_PROPERTY:${PROJECT_NAME},BINARY_DIR>/$<CONFIGURATION>/imgui.ini"
)

# copy controls.txt
file(GLOB CONTROLS_TXT controls.txt)

add_custom_target(
  ControlsTxtxFile
  DEPENDS ${CONTROLS_TXT}
)

add_dependencies(${PROJECT_NAME} ControlsTxtxFile)

add_custom_command(TARGET ControlsTxtxFile POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/controls.txt"
  "$<TARGET_PROPERTY:${PROJECT_NAME},BINARY_DIR>/$<CONFIGURATION>/controls.txt"
)

# Copy SDL2 DLLs
if (WIN32 AND NOT MINGW)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
   "${SDL2_INCLUDE_DIR}/../lib/x64/SDL2.dll"
   "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    VERBATIM)

  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
   "${SDL2_MIXER_INCLUDE_DIRS}/../lib/x64/SDL2_mixer.dll"
   "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    VERBATIM)
 endif()

# FindAssimp.cmake sets ASSIMP_LIBRARY and ASSIMP_ZLIB_LIBRARY/FindGLFW3 set GLF3_LIBRARY under Windows, reuse the value for Linux
if(UNIX)
  set(ASSIMP_LIBRARY assimp)
  set(ASSIMP_ZLIB_LIBRARY z)
  set(GLFW3_LIBRARY glfw)
endif()

# enable experimental GLM functions globally, plus add ImGui define for imnodes etc
add_definitions(-DGLM_ENABLE_EXPERIMENTAL -DIMGUI_DEFINE_MATH_OPERATORS)

# allow ImGuiFileDialog dialogs to be closed with ESC
add_definitions(-DUSE_DIALOG_EXIT_WITH_KEY -DIGFD_EXIT_KEY=ImGuiKey_Escape)

# Fix "wrong" depth in Vulkan
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

if(WIN32)
  # SDL defines its own main(), conflicts on Windows
  add_definitions(-DSDL_MAIN_HANDLED)
endif()

if(MSVC)
  target_link_libraries(${PROJECT_NAME} PRIVATE glfw ${ASSIMP_LIBRARY} ${ASSIMP_ZLIB_LIBRARY} ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARIES} Vulkan::Vulkan yaml-cpp::yaml-cpp vk-bootstrap::vk-bootstrap)
else()
  # Clang and GCC may need libstd++ and libmath
  target_link_libraries(${PROJECT_NAME} PRIVATE ${GLFW3_LIBRARY} ${ASSIMP_LIBRARY} ${ASSIMP_ZLIB_LIBRARY} ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARIES} Vulkan::Vulkan yaml-cpp vk-bootstrap::vk-bootstrap stdc++ m)
endif()

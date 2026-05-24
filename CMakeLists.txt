## CMakeLists.txt to build all examples
##
## Per-chapter build:
## Use the CMakeLists.txt files in each example's folders to build only that example

cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Mastering-Cpp-Game-Animations)

if (MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Check if Vulkan SDK is installed
find_file(bInstalledVulkanSDK NAMES vulkaninfo${CMAKE_EXECUTABLE_SUFFIX})

macro(my_add_example ChapterNum ExampleName)
	set(chap_name 		"chapter${ChapterNum}")

	set(ogl_proj_name 	"01_opengl_${ExampleName}")
	set(ogl_example 	"${chap_name}/${ogl_proj_name}")

	set(vk_proj_name 	"02_vulkan_${ExampleName}")
	set(vk_example 		"${chap_name}/${vk_proj_name}")

	add_subdirectory(${ogl_example})
	if (bInstalledVulkanSDK)
		add_subdirectory(${vk_example})
	endif()
endmacro()

if (MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
	add_subdirectory(external/assimp)
else()
	find_package(assimp REQUIRED)
endif()

add_subdirectory(external/ImGui)
add_subdirectory(external/ImGuiFileDialog)
add_subdirectory(external/stb)

if (MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
	add_subdirectory(external/glfw)
else()
	# cuz Ubuntu24.04 uses Wayland by default
	# but we dont know glfw3 will choose X11 or Wayland as backend.
	find_package(glfw3 REQUIRED)
endif()

add_subdirectory(external/glm)
add_subdirectory(external/vk-bootstrap)
add_subdirectory(external/VulkanMemoryAllocator)

# chap05+
add_subdirectory(external/yaml-cpp)

# chap09+
add_subdirectory(external/imnodes)

# chap14+
if (MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
	add_subdirectory(external/SDL2)
	add_subdirectory(external/SDL2_mixer)
else()
	find_package(SDL2 REQUIRED CONFIG)
	find_package(SDL2_mixer REQUIRED CONFIG)
endif()

#----------------------------
set(BUILD_CHAP01 ON)
set(BUILD_CHAP02 ON)
set(BUILD_CHAP03 ON)
set(BUILD_CHAP04 ON)
set(BUILD_CHAP05 ON)
set(BUILD_CHAP06 ON)
set(BUILD_CHAP07 ON)
set(BUILD_CHAP08 ON)
set(BUILD_CHAP09 ON)
set(BUILD_CHAP10 ON)
set(BUILD_CHAP11 ON)
set(BUILD_CHAP12 ON)
set(BUILD_CHAP13 ON)
set(BUILD_CHAP14 ON)

if (BUILD_CHAP01)
	my_add_example(01 assimp)
endif()
if (BUILD_CHAP02)
	my_add_example(02 computeshader)
endif()
if (BUILD_CHAP03)
	my_add_example(03 selection)
endif()
if (BUILD_CHAP04)
	my_add_example(04 edit_view_mode)
endif()
if (BUILD_CHAP05)
	my_add_example(05 load_save)
endif()
if (BUILD_CHAP06)
	my_add_example(06 cameras)
endif()
if (BUILD_CHAP07)
	my_add_example(07 animations)
endif()
if (BUILD_CHAP08)
	my_add_example(08 collisions)
endif()
if (BUILD_CHAP09)
	my_add_example(09 behavior)
endif()
if (BUILD_CHAP10)
	my_add_example(10 morphanim)
endif()
if (BUILD_CHAP11)
	my_add_example(11 level)
endif()
if (BUILD_CHAP12)
	my_add_example(12 adv_collision)
endif()
if (BUILD_CHAP13)
	my_add_example(13 navigation)
endif()
if (BUILD_CHAP14)
	my_add_example(14 ideas)
endif()
# assumpt downloaded vulkan sdk
# ref: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator

set(PROJECT_NAME VulkanMemoryAllocator)
message("==== ${PROJECT_NAME} ====")
project(${PROJECT_NAME})

option(MY_VMA_HEADER_ONLY "default ON cuz VkRenderer.cpp already defined VMA_IMPLEMENTATION" ON)
#set(MY_VMA_HEADER_ONLY OFF)

if (MY_VMA_HEADER_ONLY)
	add_library(${PROJECT_NAME} INTERFACE)
else()
	message("please manually add a .cpp file to ${CMAKE_CURRENT_SOURCE_DIR}/src")
	# vk_mem_alloc.cpp
		#define VMA_IMPLEMENTATION
		#include "vk_mem_alloc.h"
	add_library(${PROJECT_NAME} STATIC)
endif()

file(GLOB_RECURSE all_files "${CMAKE_CURRENT_SOURCE_DIR}/src/*.*")

target_include_directories(${PROJECT_NAME} INTERFACE src)
target_sources(${PROJECT_NAME} PRIVATE ${all_files})

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "external")
endif()

if(WIN32)
	set(vulkan_sdk_path $ENV{VK_SDK_PATH})
	target_include_directories(${PROJECT_NAME} INTERFACE "${vulkan_sdk_path}/Include")
	target_link_directories(   ${PROJECT_NAME} INTERFACE "${vulkan_sdk_path}/Lib")
	
	if (NOT MY_VMA_HEADER_ONLY)
		target_link_libraries(${PROJECT_NAME} PUBLIC vulkan-1)
	endif()
endif()

if (NOT MY_VMA_HEADER_ONLY)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_compile_options(${PROJECT_NAME} PRIVATE
			-Wno-error
			-Wno-unused-private-field
			-Wno-unused-variable
			-Wno-nullability-completeness)
	endif()
endif()


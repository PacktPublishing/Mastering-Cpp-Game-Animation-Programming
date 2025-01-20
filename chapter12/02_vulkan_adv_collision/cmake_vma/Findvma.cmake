# Locate the Culkan Memory Allocator - VMA
#
# This module defines the following variables:
#
# VMA_SOURCE_DIR where to find VMA source file.
# VMA_FOUND true if VMA_SOURCE_DIR have been found.
#
# To help locate the library and include file, you can define a
# variable called VMA_ROOT_DIR which points to the root of the VMA source
# installation.
#
# default search dirs
#
# Cmake file from: https://github.com/daw42/glslcookbook

set( _vma_SOURCE_SEARCH_DIRS
"/usr/include"
"/usr/local/include"
"${CMAKE_SOURCE_DIR}/includes"
"${CMAKE_SOURCE_DIR}/src"
$ENV{PROGRAMFILES}/include
$ENV{VK_SDK_PATH}/include 
${VMA_ROOT_DIR}/include )

# Check environment for root search directory
set( _vma_ENV_ROOT_DIR $ENV{VMA_ROOT_DIR} )
if( NOT VMA_ROOT_DIR AND _vma_ENV_ROOT_DIR )
	set(VMA_ROOT_DIR ${_vma_ENV_ROOT_DIR} )
endif()

# Put user specified location at beginning of search
if( VMA_ROOT_DIR )
	list( INSERT _vma_SOURCE_SEARCH_DIRS 0 "${VMA_ROOT_DIR}/include" )
endif()

# Search for the source file
FIND_PATH(VMA_SOURCE_DIR "vma/vk_mem_alloc.h"
PATHS ${_vma_SOURCE_SEARCH_DIRS} )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(vma DEFAULT_MSG VMA_SOURCE_DIR)

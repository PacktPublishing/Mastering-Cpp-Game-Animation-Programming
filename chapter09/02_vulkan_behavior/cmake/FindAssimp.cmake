# Locate the Open Asset Importer Library - assimp
#
# This module defines the following variables:
#
# ASSIMP_LIBRARY the name of the library;
# ASSIMP_LIBZ_LIBRARY the name of the static libz library
# ASSIMP_INCLUDE_DIR where to find Assimp include files.
# ASSIMP_FOUND true if both the ASSIMP_LIBRARY and ASSIMP_INCLUDE_DIR have been found.
#
# To help locate the library and include file, you can define a
# variable called ASSIMP_ROOT which points to the root of the Assimp library
# installation.
#
# default search dirs
#
# Cmake file from: https://github.com/daw42/glslcookbook

set( _assimp_HEADER_SEARCH_DIRS
"/usr/include"
"/usr/local/include"
"${CMAKE_SOURCE_DIR}/includes"
"C:/Program Files/assimp/include" )

set( _assimp_LIB_SEARCH_DIRS
"/usr/lib"
"/usr/local/lib"
"${CMAKE_SOURCE_DIR}/lib"
"C:/Program Files/assimp/lib" )

# Check environment for root search directory
set( _assimp_ENV_ROOT $ENV{ASSIMP_ROOT} )
if( NOT ASSIMP_ROOT AND _assimp_ENV_ROOT )
	set(ASSIMP_ROOT ${_assimp_ENV_ROOT} )
endif()

# Put user specified location at beginning of search
if( ASSIMP_ROOT )
	list( INSERT _assimp_HEADER_SEARCH_DIRS 0 "${ASSIMP_ROOT}/include" )
	list( INSERT _assimp_LIB_SEARCH_DIRS 0 "${ASSIM_ROOT}/lib" )
endif()

# Search for the header
FIND_PATH(ASSIMP_INCLUDE_DIR "assimp/Importer.hpp"
PATHS ${_assimp_HEADER_SEARCH_DIRS} )

# Search for the libraies
FIND_LIBRARY(ASSIMP_LIBRARY NAMES assimp
PATHS ${_assimp_LIB_SEARCH_DIRS} )

# Search for the libraies
FIND_LIBRARY(ASSIMP_ZLIB_LIBRARY NAMES zlibstatic libz
PATHS ${_assimp_LIB_SEARCH_DIRS} )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(assimp DEFAULT_MSG ASSIMP_LIBRARY ASSIMP_ZLIB_LIBRARY ASSIMP_INCLUDE_DIR)

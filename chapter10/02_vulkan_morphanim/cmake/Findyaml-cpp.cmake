# Locate yaml-cpp
#
# This module defines
#  yaml-cpp_FOUND, if false, do not try to link to yaml-cpp
#  yaml-cpp_LIBRARY, where to find yaml-cpp
#  yaml-cpp_INCLUDE_DIR, where to find yaml.h
#
# By default, the dynamic libraries of yaml-cpp will be found. To find the static ones instead,
# you must set the yaml-cpp_STATIC_LIBRARY variable to TRUE before calling find_package(yaml-cpp ...).
#
# If yaml-cpp is not installed in a standard path, you can use the yaml-cpp_DIR CMake variable
# to tell CMake where yaml-cpp is.

# attempt to find static library first if this is set
if(yaml-cpp_STATIC_LIBRARY)
    set(yaml-cpp_STATIC libyaml-cpp.a)
endif()


# find the yaml-cpp include directory
find_path(yaml-cpp_INCLUDE_DIR yaml-cpp/yaml.h
          PATHS ${yaml-cpp_DIR}/include/)

# find the yaml-cpp library
find_library(yaml-cpp_LIBRARY
             NAMES ${yaml-cpp_STATIC} yaml-cpp
             PATHS  ${yaml-cpp_DIR}/lib)

# handle the QUIETLY and REQUIRED arguments and set yaml-cpp_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(yaml-cpp DEFAULT_MSG yaml-cpp_INCLUDE_DIR yaml-cpp_LIBRARY)
mark_as_advanced(yaml-cpp_INCLUDE_DIR yaml-cpp_LIBRARY)

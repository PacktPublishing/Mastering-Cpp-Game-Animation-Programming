set(PROJECT_NAME stb)
message("==== ${PROJECT_NAME} ====")
project(${PROJECT_NAME})

add_library(${PROJECT_NAME} STATIC)

file(GLOB_RECURSE all_files "${CMAKE_CURRENT_SOURCE_DIR}/src/*.*")

target_include_directories(${PROJECT_NAME} PUBLIC src/stb)
target_sources(${PROJECT_NAME} PRIVATE ${all_files})

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "external")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${PROJECT_NAME} PRIVATE
		-Wno-switch-default)
endif()
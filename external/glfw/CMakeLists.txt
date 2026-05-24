set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS 	OFF)
set(GLFW_BUILD_DOCS 	OFF)
set(GLFW_INSTALL 		OFF)

set(GLFW_LIBRARY_TYPE "STATIC")

add_subdirectory(glfw-3.4)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set_target_properties (glfw 			PROPERTIES FOLDER "external/glfw")
	set_target_properties (update_mappings 	PROPERTIES FOLDER "external/glfw")
endif()



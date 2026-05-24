set(VK_BOOTSTRAP_TEST OFF)
set(VK_BOOTSTRAP_INSTALL OFF)

add_subdirectory(vk-bootstrap-1.3.302)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set_target_properties (vk-bootstrap PROPERTIES FOLDER "external")
endif()



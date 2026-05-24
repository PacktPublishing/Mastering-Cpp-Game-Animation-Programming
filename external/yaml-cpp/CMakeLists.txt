set(YAML_BUILD_SHARED_LIBS 	OFF)
set(YAML_CPP_INSTALL 		OFF)
set(YAML_ENABLE_PIC 		OFF) # static lib no need pic
set(YAML_CPP_BUILD_TESTS  	OFF)

add_subdirectory(yaml-cpp-yaml-cpp-0.9.0)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set_target_properties (yaml-cpp 		PROPERTIES FOLDER "external/yaml-cpp")
	set_target_properties (yaml-cpp-parse 	PROPERTIES FOLDER "external/yaml-cpp")
	set_target_properties (yaml-cpp-read 	PROPERTIES FOLDER "external/yaml-cpp")
	set_target_properties (yaml-cpp-sandbox PROPERTIES FOLDER "external/yaml-cpp")
endif()

if (MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
	target_compile_options(yaml-cpp INTERFACE /EHsc)
else()
    target_compile_options(yaml-cpp INTERFACE -fexceptions)
endif()
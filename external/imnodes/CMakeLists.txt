set(IMNODES_EXAMPLES OFF)
set(IMNODES_IMGUI_TARGET_NAME imgui::imgui)

set(BUILD_SHARED_LIBS OFF) # force as static lib
	add_subdirectory(imnodes-b2ec254ce576ac3d42dfb7aef61deadbff8e7211)
unset(BUILD_SHARED_LIBS)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set_target_properties (imnodes PROPERTIES FOLDER "external")
endif()
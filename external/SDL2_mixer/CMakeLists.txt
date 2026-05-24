# dl source from github: https://github.com/libsdl-org/SDL_mixer/releases/tag/release-2.8.0
	# dl link: https://github.com/libsdl-org/SDL_mixer/archive/refs/tags/release-2.8.0.zip
# after downloaded unzip SDL_mixer-release-2.8.0.zip to this folder
# then run "download_SDL_mixer_deps.bat/.sh"

set(BUILD_SHARED_LIBS 	ON)
set(SDL2MIXER_SAMPLES 	OFF)

set(SDL2MIXER_VENDORED  ON) # ON means already called "download_SDL_mixer_deps" shell
set(SDL2MIXER_OPUS 		OFF)
#set(SDL2MIXER_WAVPACK 	ON)

add_subdirectory(SDL_mixer-release-2.8.0)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set_target_properties (SDL2_mixer 	PROPERTIES FOLDER "external/SDL2_mixer")
	
	if(SDL2MIXER_OPUS)
		set_target_properties (ogg 			PROPERTIES FOLDER "external/SDL2_mixer")
		set_target_properties (opus 		PROPERTIES FOLDER "external/SDL2_mixer")
		set_target_properties (opusfile 	PROPERTIES FOLDER "external/SDL2_mixer")
	endif()
	if(SDL2MIXER_WAVPACK)
		set_target_properties (wavpack 		PROPERTIES FOLDER "external/SDL2_mixer")
	endif()
	if(SDL2MIXER_MOD_XMP)
		set_target_properties (xmp_shared 	PROPERTIES FOLDER "external/SDL2_mixer")
	endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(SDL2_mixer PRIVATE
		-Wno-static-in-inline
		-Wno-unused-but-set-variable
		-Wno-sign-compare)

	if(SDL2MIXER_MOD_XMP)
		target_compile_options(xmp_shared PRIVATE
			-Wno-sign-compare)
	endif()
endif()

# ------------ deps
if (BUILD_SHARED_LIBS)
	add_dependencies(SDL2_mixer SDL2)
	target_link_libraries(SDL2_mixer PUBLIC SDL2::SDL2)
else()
	add_dependencies(SDL2_mixer SDL2-static)
	target_link_libraries(SDL2_mixer PUBLIC SDL2::SDL2-static)
endif()

#ref: https://github.com/libsdl-org/SDL/releases/tag/release-2.30.9
#dl link: https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.30.9.zip
	# after downloaded then unzip SDL-release-2.30.9.zip to this folder

set(SDL_SHARED 				ON)
set(SDL_STATIC 				OFF)
set(SDL_TEST   				OFF)
set(SDL2_DISABLE_SDL2MAIN   ON) # no need SDL2main

add_subdirectory(SDL-release-2.30.9)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set_target_properties (sdl_headers_copy PROPERTIES FOLDER "external/sdl2")
	if(SDL_SHARED)
		set_target_properties (SDL2 		PROPERTIES FOLDER "external/sdl2")
	else()
		set_target_properties (SDL2-static 	PROPERTIES FOLDER "external/sdl2")
	endif()
	
	if (NOT SDL2_DISABLE_SDL2MAIN)
		set_target_properties (SDL2main 	PROPERTIES FOLDER "external/sdl2")
	endif()
endif()
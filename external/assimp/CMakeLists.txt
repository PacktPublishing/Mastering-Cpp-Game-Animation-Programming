# git clone assimp-master source from github: https://github.com/assimp/assimp
# and unzip to this folder
# then run gen_vs2022.bat

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-Wno-unused-private-field
						-Wno-unused-variable
						-Wno-unused-member-function
						-Wno-unused-command-line-argument
						#-Wno-error=character-conversion
						#-Wno-character-conversion
						-Wno-format)
endif()

set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_BUILD_TEST    OFF)
set(ASSIMP_BUILD_DOCS    OFF)

add_subdirectory(assimp-master)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set_target_properties (UpdateAssimpLibsDebugSymbolsAndDLLs  PROPERTIES FOLDER "external/assimp")
	set_target_properties (assimp     							PROPERTIES FOLDER "external/assimp")
	set_target_properties (unit       							PROPERTIES FOLDER "external/assimp")
	set_target_properties (zlibstatic 							PROPERTIES FOLDER "external/assimp")
	set_target_properties (uninstall 							PROPERTIES FOLDER "external/assimp")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(assimp PRIVATE
		-Wno-error
		-Wno-unsafe-buffer-usage
		-Wno-switch-default
		-Wno-unreachable-code-break)
	
	target_compile_options(unit PRIVATE
		-Wno-deprecated-copy)

	if(CMAKE_GENERATOR MATCHES "Visual Studio")
		target_compile_options(zlibstatic PRIVATE
			-Wno-deprecated-non-prototype)
	endif()
endif()
set(PROJECT_NAME 02_vulkan_computeshader)
message("==== ${PROJECT_NAME} ====")
project(${PROJECT_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/_bin/$<CONFIGURATION>/${PROJECT_NAME}")

get_filename_component(MY_PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(MY_PARENT_DIR_NAME ${MY_PARENT_DIR} NAME)

find_package(Vulkan REQUIRED)

# optimize for speed on Clang or GCC
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "-O3")
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  message("-- Using ccache")
  set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif(CCACHE_FOUND)

file(GLOB SOURCES
  Main.cpp
  window/*.cpp
  tools/*.cpp
  vulkan/*.cpp
  model/*.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC window tools vulkan model)

# compile shaders
set(MY_SHADERS_NAME "Shaders_${PROJECT_NAME}")
file(GLOB GLSL_SOURCE_FILES
  shader/*.frag
  shader/*.vert
  shader/*.comp
)

if(Vulkan_GLSLC_EXECUTABLE)
  message("Using glslc to compile shaders")
  foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${Vulkan_GLSLC_EXECUTABLE} -o ${SPIRV} ${GLSL}
      DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  endforeach(GLSL)
elseif (Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
  message("Using glslangValidator to compile shaders")
  foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V -o ${SPIRV} ${GLSL}
      DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  endforeach(GLSL)
endif()

add_custom_target(
  ${MY_SHADERS_NAME}
  DEPENDS ${SPIRV_BINARY_FILES}
)
add_dependencies(${PROJECT_NAME} ${MY_SHADERS_NAME})

add_custom_command(TARGET ${MY_SHADERS_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/shader"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader"
)

# copy textures
set(MY_TEXTURES_NAME "Textures_${PROJECT_NAME}")
file(GLOB TEX_SOURCE_FILES
  textures/*
)

add_custom_target(
  ${MY_TEXTURES_NAME}
  DEPENDS ${TEX_SOURCE_FILES}
)
add_dependencies(${PROJECT_NAME} ${MY_TEXTURES_NAME})

add_custom_command(TARGET ${MY_TEXTURES_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/textures"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/textures"
)

# copy assets
set(MY_ASSETS_NAME "Assets_${PROJECT_NAME}")
file(GLOB ASSET_FILES
  assets/*
)

add_custom_target(
  ${MY_ASSETS_NAME}
  DEPENDS ${ASSET_FILES}
)

add_dependencies(${PROJECT_NAME} ${MY_ASSETS_NAME})

add_custom_command(TARGET ${MY_ASSETS_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/assets"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
)

# copy imgui.ini
set(MY_IMGUI_INI_NAME "ImGuiIniFile_${PROJECT_NAME}")
file(GLOB IMGUI_INI imgui.ini)

add_custom_target(
  ${MY_IMGUI_INI_NAME}
  DEPENDS ${IMGUI_INI}
)

add_dependencies(${PROJECT_NAME} ${MY_IMGUI_INI_NAME})

add_custom_command(TARGET ${MY_IMGUI_INI_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  "$<TARGET_PROPERTY:${PROJECT_NAME},SOURCE_DIR>/imgui.ini"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/imgui.ini"
)

# enable experimental GLM functions globally
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# Fix "wrong" depth in Vulkan
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

target_link_libraries(${PROJECT_NAME} PRIVATE
	glfw
	Vulkan::Vulkan
	vk-bootstrap::vk-bootstrap
	assimp
	ImGuiFileDialog
	imgui
	stb
	VulkanMemoryAllocator
	glm
)

if(MSVC)
	# do nothing
else()
  # Clang and GCC may need libstd++ and libmath
  target_link_libraries(${PROJECT_NAME} PRIVATE stdc++ m z)
endif()

if (MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
	# copy assimp dll
	set(MY_ASSIMP_DLL_NAME "assimp-vc143-mt.dll")
	set(MY_ASSIMP_DLL "external/assimp/_build/vs2022-x64-windows/bin/$<CONFIGURATION>/${MY_ASSIMP_DLL_NAME}")
	set(MY_ASSIMP_NAME "assimpdll_${PROJECT_NAME}")

	add_custom_target(
	  ${MY_ASSIMP_NAME}
	  DEPENDS ${MY_ASSIMP_DLL}
	)
	add_dependencies(${PROJECT_NAME} ${MY_ASSIMP_NAME})
	add_custom_command(TARGET ${MY_ASSIMP_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_SOURCE_DIR}/${MY_ASSIMP_DLL}"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MY_ASSIMP_DLL_NAME}"
	)
endif()

if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set(MY_VS_FOLDER "${MY_PARENT_DIR_NAME}/${PROJECT_NAME}")
	set_target_properties (${PROJECT_NAME}  	  PROPERTIES FOLDER ${MY_VS_FOLDER})
	set_target_properties (${MY_ASSETS_NAME}    PROPERTIES FOLDER ${MY_VS_FOLDER})
	set_target_properties (${MY_SHADERS_NAME} 	PROPERTIES FOLDER ${MY_VS_FOLDER})
	set_target_properties (${MY_TEXTURES_NAME}  PROPERTIES FOLDER ${MY_VS_FOLDER})
	set_target_properties (${MY_IMGUI_INI_NAME} PROPERTIES FOLDER ${MY_VS_FOLDER})
	set_target_properties (${MY_ASSIMP_NAME}  	PROPERTIES FOLDER ${MY_VS_FOLDER})

	# set working dir
	set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()
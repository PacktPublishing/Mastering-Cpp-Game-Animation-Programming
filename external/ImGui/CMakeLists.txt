set(PROJECT_NAME imgui)
message("==== ${PROJECT_NAME} ====")
project(${PROJECT_NAME})

add_library(${PROJECT_NAME} STATIC)
add_library(imgui::imgui ALIAS ${PROJECT_NAME})

file(GLOB_RECURSE all_files "${CMAKE_CURRENT_SOURCE_DIR}/src/*.*")

target_include_directories(${PROJECT_NAME} PUBLIC src/imgui)
target_include_directories(${PROJECT_NAME} PUBLIC src/imgui/backends)

target_sources(${PROJECT_NAME} PRIVATE ${all_files})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "external")

# ------------ backends deps
# imgui_impl_glfw
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# imgui_impl_vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
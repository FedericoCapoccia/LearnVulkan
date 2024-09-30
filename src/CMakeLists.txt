add_executable(${CMAKE_PROJECT_NAME}
        application.cpp
        gfx_manager.cpp
        vk_utils.cpp
        window.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR} ${Vulkan_INCLUDE_DIR}")
target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE pch.hpp)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glm ${Vulkan_LIBRARY} glfw fmt::fmt)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE)

add_dependencies(${CMAKE_PROJECT_NAME} Shaders)
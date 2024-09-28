add_executable(${CMAKE_PROJECT_NAME}
        main.cpp
        engine.cpp
        window.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR} ${Vulkan_INCLUDE_DIR}")

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glm ${Vulkan_LIBRARY} SDL3::SDL3)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE)

add_dependencies(${CMAKE_PROJECT_NAME} Shaders)
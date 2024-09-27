cmake_minimum_required(VERSION 3.29)
project(Minecraft VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)

add_executable(${CMAKE_PROJECT_NAME}
        src/main.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        include
        ${glad_SOURCE_DIR}/include
        third-party/stb_image
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        glfw
        glad
#        glm::glm
)
if (WIN32)
    target_link_libraries(${CMAKE_PROJECT_NAME} opengl32)
endif()

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra -fno-exceptions)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE)

add_custom_target(copy_resources COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources)
add_dependencies(${CMAKE_PROJECT_NAME} copy_resources)

#-------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------
add_subdirectory("third-party/glad")

include(FetchContent)
#-------------------------------------------------------------------------
# Fetch GLM
# ------------------------------------------------------------------------
#set(GLM_VERSION 1.0.1)
#message(STATUS "include GLM")
#FetchContent_Declare(
#        libglm
#        GIT_REPOSITORY https://github.com/g-truc/glm.git
#        GIT_TAG ${GLM_VERSION}
#        GIT_SHALLOW TRUE
#        GIT_PROGRESS TRUE
#)
#
#FetchContent_MakeAvailable(libglm)


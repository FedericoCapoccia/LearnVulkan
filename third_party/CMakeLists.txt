#-------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------
include(FetchContent)
set(GLM_VERSION 1.0.1)
set(GLFW_VERSION 3.4)

#-------------------------------------------------------------------------
# Fetch GLM
# ------------------------------------------------------------------------
message(STATUS "include GLM")
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG ${GLM_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glm)

#-------------------------------------------------------------------------
# GLFW
# ------------------------------------------------------------------------
message(STATUS "include GLFW")
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG ${GLFW_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

set(GLFW_INSTALL OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
FetchContent_MakeAvailable(glfw)

#-------------------------------------------------------------------------
# Vulkan
# ------------------------------------------------------------------------
message(STATUS "Retrieving Vulkan")
find_package(Vulkan REQUIRED)
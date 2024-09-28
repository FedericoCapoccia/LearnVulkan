#-------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------
include(FetchContent)
set(GLM_VERSION 1.0.1)
set(SDL_VERSION main)

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
# Fetch SDL
# ------------------------------------------------------------------------
message(STATUS "include SDL3")
set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG ${SDL_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL)

#-------------------------------------------------------------------------
# Vulkan
# ------------------------------------------------------------------------
message(STATUS "Retrieving Vulkan")
find_package(Vulkan REQUIRED)
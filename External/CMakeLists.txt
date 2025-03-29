include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)

FetchContent_MakeAvailable(SDL3)

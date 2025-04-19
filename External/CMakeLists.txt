include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
    EXCLUDE_FROM_ALL
)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)

FetchContent_MakeAvailable(SDL3)

if(WIN32)
    FetchContent_Declare(
        DirectX-Headers
        GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
        GIT_TAG v1.615.0
    )

    FetchContent_MakeAvailable(DirectX-Headers)
endif()

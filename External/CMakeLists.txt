include(FetchContent)

if(WIN32)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.32.4
        EXCLUDE_FROM_ALL
    )
    set(SDL_SHARED OFF)
    set(SDL_STATIC ON)
    FetchContent_MakeAvailable(SDL2)

    FetchContent_Declare(
        DirectX-Headers
        GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
        GIT_TAG v1.615.0
    )

    FetchContent_MakeAvailable(DirectX-Headers)
endif()

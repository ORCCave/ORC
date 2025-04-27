include(FetchContent)

if(WIN32)
    set(SDL_SHARED OFF)
    set(SDL_STATIC ON)
    FetchContent_Declare(
        SDL3
         https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.10
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3)

    FetchContent_Declare(
        DirectX-Headers
        GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
        GIT_TAG v1.615.0
    )

    FetchContent_MakeAvailable(DirectX-Headers)
endif()

if(UNIX AND NOT APPLE)
    find_package(SDL REQUIRED)
endif()

# ORC

[![windows](https://github.com/ORCCave/ORC/actions/workflows/windows.yml/badge.svg)](https://github.com/ORCCave/ORC/actions/workflows/windows.yml)
[![linux](https://github.com/ORCCave/ORC/actions/workflows/linux.yml/badge.svg)](https://github.com/ORCCave/ORC/actions/workflows/linux.yml)

ORC (Object-Oriented Rendering Component)

| Platform | Supported APIs                  |
|----------|---------------------------------|
| Windows  | DX12, Vulkan                    |
| Linux    | Vulkan                          |

## Prerequisites

To successfully build this project, ensure the following tools are installed on your system:

- **CMake** (version 3.25 or higher)
- **Git** (required for downloading external dependencies like SDL2)

### Why Git is Required

This project uses CMake’s **FetchContent** module, which relies on Git to automatically download external dependencies (e.g., SDL2). If Git is not installed or not accessible in your system’s PATH, you’ll encounter:

`error: could not find git for clone of sdl2-populate`

### For Linux Users: Install Required Dependencies

On Linux, install **Vulkan** and **SDL2**.

Ubuntu:

`sudo apt-get install libvulkan-dev libsdl2-dev`

## Third Party Libraries

- [SDL2](https://github.com/libsdl-org/SDL) - (Zlib license)
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - (MIT license)
- [DirectX Headers](https://github.com/microsoft/DirectX-Headers) - (MIT license)

## License

MIT license

# ORC

[![windows](https://github.com/ORCCave/ORC/actions/workflows/windows.yml/badge.svg)](https://github.com/ORCCave/ORC/actions/workflows/windows.yml)
[![linux](https://github.com/ORCCave/ORC/actions/workflows/linux.yml/badge.svg)](https://github.com/ORCCave/ORC/actions/workflows/linux.yml)

ORC (Object-Oriented Rendering Component)

| Platform | Supported APIs                  |
|----------|---------------------------------|
| Windows  | DX12, Vulkan (optional)         |
| Linux    | Vulkan                          |


## Prerequisites

To successfully build this project, ensure the following tools are installed on your system:

- **CMake** (version 3.25 or higher)
- **Git** (required for downloading external dependencies like SDL3)

### Why Git is Required
This project uses CMake’s `FetchContent` module, which relies on Git to automatically download external dependencies (e.g., SDL3). If Git is not installed or not accessible in your system’s PATH, you’ll encounter `error: could not find git for clone of sdl3-populate`.

### For Linux Users: Install Required Dependencies
On Linux, **Vulkan** are mandatory. Additionally, **X11** and **Xext** packages are required for display support. For Ubuntu/Debian, install the dependencies with:
```bash
sudo apt-get install libx11-dev libxext-dev libvulkan-dev
```
For other Linux distributions, the required packages are similar. For other operating systems, Vulkan support is available as an optional component.

## Third Party Libraries
 * [SDL](https://github.com/libsdl-org/SDL) - (Zlib license)

## License

MIT license

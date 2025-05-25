cmake_minimum_required(VERSION 3.28)
project(ORC)

set(CMAKE_CXX_STANDARD 20)

if(NOT(WIN32))
    message(FATAL_ERROR "Orc does not support this platform")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")

add_subdirectory("OrcMain")
add_subdirectory("Samples")
cmake_minimum_required(VERSION 3.28)
project(ORC)

set(CMAKE_CXX_STANDARD 20)

if(NOT(WIN32))
    message(FATAL_ERROR "Orc does not support this platform")
endif()

add_subdirectory("OrcMain")
add_subdirectory("Samples")
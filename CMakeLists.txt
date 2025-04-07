cmake_minimum_required(VERSION 3.28)
project(ORC)

set(CMAKE_CXX_STANDARD 23)

if(NOT(WIN32 OR (UNIX AND NOT APPLE)))
    message(FATAL_ERROR "Orc does not support this platform")
endif()

add_subdirectory("External")
add_subdirectory("OrcMain")
add_subdirectory("Samples")
cmake_minimum_required(VERSION 3.28)
project(ORC)

set(CMAKE_CXX_STANDARD 20)

if(APPLE OR (NOT WIN32 AND NOT UNIX))
	message(FATAL_ERROR "Orc does not support this platform")
endif()

add_subdirectory("External")
add_subdirectory("OrcMain")
add_subdirectory("Samples")
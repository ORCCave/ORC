file(GLOB_RECURSE ORC_SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE ORC_HEADER_FILES CONFIGURE_DEPENDS *.h)

add_library(OrcMain STATIC ${ORC_SOURCE_FILES} ${ORC_HEADER_FILES})

target_include_directories(OrcMain PUBLIC "include")
target_include_directories(OrcMain PRIVATE "${PROJECT_SOURCE_DIR}/External/tinygltf/include")
target_precompile_headers(OrcMain PRIVATE "src/OrcPrerequisites.h")

add_custom_command(
    TARGET OrcMain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "${PROJECT_SOURCE_DIR}/OrcAssets/Models" 
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Models"
)
add_executable(Window "Window/Window.cpp")
target_link_libraries(Window PRIVATE OrcMain)

add_executable(Model "Model/Model.cpp")
target_link_libraries(Model PRIVATE OrcMain)
set_target_properties(Model PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/OrcAssets")
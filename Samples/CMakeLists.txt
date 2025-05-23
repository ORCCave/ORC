add_executable(Window "Window/Window.cpp")
target_link_libraries(Window PRIVATE OrcMain)

add_executable(Model "Model/Model.cpp")
target_link_libraries(Model PRIVATE OrcMain)
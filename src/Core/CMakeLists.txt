add_library(core
    Window.cpp
    Log.cpp
    InputSystem.cpp
    Camera.cpp
)

target_compile_options(core PRIVATE /utf-8)

target_include_directories(core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_include_directories(core
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core
    PUBLIC spdlog platform glm
)

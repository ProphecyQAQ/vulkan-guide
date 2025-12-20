add_library(core
    Window.cpp
)

target_include_directories(core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_include_directories(core
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
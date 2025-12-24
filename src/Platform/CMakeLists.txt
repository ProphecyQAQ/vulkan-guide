add_library(platform
    Windows/WindowsWindow.cpp
)

target_include_directories(platform
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(platform
    PUBLIC core SDL2::SDL2
)
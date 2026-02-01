add_library(scene
    Scene.cpp
    Object.cpp
)

target_include_directories(scene
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(scene
    PUBLIC core glm fastgltf::fastgltf rhi
)
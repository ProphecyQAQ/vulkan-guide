add_library(rhi
    GraphicsContext.cpp
)

add_subdirectory(Vulkan)
add_subdirectory(RDG)

target_include_directories(rhi
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(rhi
    PUBLIC core VulkanRHI RDG
)
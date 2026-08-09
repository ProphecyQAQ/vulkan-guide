add_library(rdg
    RDGPass.cpp
    RDGContext.cpp
    RDGResource.cpp
    RDGGraph.cpp
)

target_include_directories(rhi
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(rhi
    PUBLIC core VulkanRHI
)
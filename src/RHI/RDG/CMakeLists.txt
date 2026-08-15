add_library(RDG
    RDGPass.cpp
    RDGContext.cpp
    RDGResource.cpp
    RDGGraph.cpp
    RDGTransientResourcePool.cpp
)

target_include_directories(RDG
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(RDG
    PUBLIC core VulkanRHI
)
add_library(VulkanRHI
    VulkanContext.cpp
    VulkanSwapChain.cpp
    VulkanDevice.cpp
    VulkanCommandBuffer.cpp
    VulkanDescriptorSet.cpp
    VulkanPipeline.cpp
    VulkanHelper.cpp
)

target_include_directories(VulkanRHI
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_definitions(VulkanRHI PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_link_libraries(VulkanRHI PUBLIC core vma magic_enum glm Vulkan::Vulkan SDL2::SDL2 vkbootstrap imgui)
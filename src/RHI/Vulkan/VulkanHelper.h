#pragma once
#include <Vulkan/VulkanPCH.h>
#include <Core/Log.h>

class VulkanHeaplerLibrary 
{
public:
    static std::vector<VkPhysicalDevice> getPhysicalDevices(VkInstance instance);
    static std::vector<VkQueueFamilyProperties> getQueueFamily(VkPhysicalDevice physicalDevice);

    static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    static VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes);

    static VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);
    static VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

    static VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char * entry = "main");
    static VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(VkPipelineLayoutCreateFlags flags = 0);

    static bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);

    // cmd
    static VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags /*= 0*/);
    static VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);

    // Image
    static void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
    static VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);
    static void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
    // Image

    // sync
    static VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

    // submit
    static VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo = nullptr,
        VkSemaphoreSubmitInfo* waitSemaphoreInfo = nullptr);
};
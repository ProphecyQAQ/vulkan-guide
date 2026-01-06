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
};
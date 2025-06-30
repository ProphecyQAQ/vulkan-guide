#pragma once

#include <vk_types.h>

class VulkanHeaplerLibrary 
{
public:
    static std::vector<VkPhysicalDevice> get_physical_devices(VkInstance instance);
    static std::vector<VkQueueFamilyProperties> get_queue_family(VkPhysicalDevice physicalDevice);

    static SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    static VkSurfaceFormatKHR select_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR select_swap_present_mode(const std::vector<VkPresentModeKHR>& availableModes);
};
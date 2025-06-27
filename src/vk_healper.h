#pragma once

#include <vk_types.h>

class VulkanHeaplerLibrary 
{
public:
    static std::vector<VkPhysicalDevice> get_physical_devices(VkInstance instance);
    static std::vector<VkQueueFamilyProperties> get_queue_family(VkPhysicalDevice physicalDevice);
};
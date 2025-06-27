#pragma once

#include <vk_types.h>

class VulkanHeaplerLibrary 
{
public:
    static std::vector<VkPhysicalDevice> GetPhysicalDevices(VkInstance instance);
};
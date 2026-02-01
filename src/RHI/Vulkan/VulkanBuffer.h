#pragma once

#include <Vulkan/VulkanPCH.h>

class VulkanBuffer
{
public:
    VulkanBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    ~VulkanBuffer();
private:
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
};
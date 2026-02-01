#pragma once

#include <Vulkan/VulkanPCH.h>

class VulkanBuffer
{
public:
    VulkanBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    ~VulkanBuffer();

    void* getMappedData() { return allocationInfo.pMappedData; }
    VkBuffer getBuffer() const { return buffer; }
    VkDeviceAddress getBufferAddress() const { return vertexBufferAddress; }
private:
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

    VkDeviceAddress vertexBufferAddress;
};
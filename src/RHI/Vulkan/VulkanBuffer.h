#pragma once

#include <Vulkan/VulkanPCH.h>

class VulkanBuffer
{
public:
    VulkanBuffer() = default;
    VulkanBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    ~VulkanBuffer();

    void init(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    void* getMappedData() { return allocationInfo.pMappedData; }
    VkBuffer getBuffer() const { return buffer; }
    VkDeviceAddress getBufferAddress() const { return vertexBufferAddress; }

    template<typename T>
    void setData(const T* data)
    {
        assert(sizeof(T) == allocationInfo.size);

        T* mappedData = (T*)getMappedData();
        *mappedData = *data;
    }
private:
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

    VkDeviceAddress vertexBufferAddress;
};
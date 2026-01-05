#pragma once

#include <Vulkan/VulkanDevice.h>
#include <Vulkan/VulkanPCH.h>

class VulkanCommandBuffer;

enum VulkanCommandBufferType
{
    VK_CMD_BUFFER_TYPE_PRIMARY,
    VK_CMD_BUFFER_TYPE_SECONDARY
};

class VulkanCommandBufferPool
{
public:
    VulkanCommandBufferPool(VulkanDevice& device, VulkanCommandBufferType type);
    ~VulkanCommandBufferPool();

    VulkanCommandBuffer* create();
    VulkanCommandBufferType getType() const { return type; }
    VkCommandPool getHandle() const { return cmdPool; }
private:
    std::vector<VulkanCommandBuffer*> cmdBuffers;
    VkCommandPool cmdPool;

    VulkanDevice& device;
    uint32_t queueFamilyIndex;
    VulkanCommandBufferType type;
};

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer(VulkanDevice& device, VulkanCommandBufferPool& cmdPool);
    ~VulkanCommandBuffer();

    VkCommandBuffer getHandle() const { return commandBuffer; }
    VulkanCommandBufferType getType() const { return cmdPool.getType(); }
private:
    VulkanDevice& device;
    VulkanCommandBufferPool& cmdPool;
    VkCommandBuffer commandBuffer;
};

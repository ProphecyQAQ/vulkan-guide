#include <Vulkan/VulkanCommandBuffer.h>

// ------------------------------ VulkanCommandBuffer -----------------------
VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device, VulkanCommandBufferPool& cmdPool)
    : device(device), cmdPool(cmdPool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmdPool.getHandle();
    allocInfo.level = (getType() == VK_CMD_BUFFER_TYPE_PRIMARY) ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(device.getDevice(), &allocInfo, &commandBuffer));
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    vkFreeCommandBuffers(device.getDevice(), cmdPool.getHandle(), 1, &commandBuffer);
}
// ------------------------------ VulkanCommandBuffer -----------------------

// ------------------------------ VulkanCommandBufferPool -----------------------
VulkanCommandBufferPool::VulkanCommandBufferPool(VulkanDevice& device, VulkanCommandBufferType type)
    : device(device), queueFamilyIndex(device.getGraphicsQueueFamilyIndex()), type(type)
{
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;

    VK_CHECK(vkCreateCommandPool(device.getDevice(), &info, nullptr, &cmdPool));
}

VulkanCommandBufferPool::~VulkanCommandBufferPool()
{
    for (VulkanCommandBuffer* cmdBuffer : cmdBuffers)
    {
        delete cmdBuffer;
    }
    vkDestroyCommandPool(device.getDevice(), cmdPool, nullptr); 
}

VulkanCommandBuffer* VulkanCommandBufferPool::create()
{
    VulkanCommandBuffer* cmdBuffer = new VulkanCommandBuffer(device, *this);
    cmdBuffers.push_back(cmdBuffer);
    return cmdBuffer;
}
// ------------------------------ VulkanCommandBufferPool -----------------------
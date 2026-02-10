#include <Vulkan/VulkanBuffer.h>
#include <Vulkan/VulkanContext.h>

VulkanBuffer::VulkanBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    init(allocSize, usage, memoryUsage);
}

void VulkanBuffer::init(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = memoryUsage;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VK_CHECK(vmaCreateBuffer(VulkanContext::get()->getAllocator(), &bufferInfo, &allocCreateInfo, &buffer, &allocation, &allocationInfo));

    VkBufferDeviceAddressInfo vertexAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer};
    vertexBufferAddress = vkGetBufferDeviceAddress(VulkanContext::get()->getVkDevice(), &vertexAddressInfo);
}

VulkanBuffer::~VulkanBuffer()
{
    if (buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(VulkanContext::get()->getAllocator(), buffer, allocation);
    }
}
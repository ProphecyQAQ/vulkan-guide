#pragma once
#include <string>
#include <Vulkan/VulkanPCH.h>
#include <Vulkan/VulkanBuffer.h>
#include <Vulkan/VulkanImage.h>

enum RDGResourceType
{
    NONE,
    RDG_RESOURCE_TYPE_TEXTURE,
    RDG_RESOURCE_TYPE_BUFFER,
};

struct RDGBufferDesc
{
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
}; 

struct RDGTextureDesc
{
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent3D extent = { 0, 0, 1};
    VkImageUsageFlags usage = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
    bool mipmapped = false;
};

struct RDGResource
{
    uint32_t handle = 0;

    std::string name;
    enum RDGResourceType type = RDGResourceType::NONE;

    // resource desc
    union
    {
        RDGBufferDesc bufferDesc;
        RDGTextureDesc textureDesc;
    };

    // runtime resource
    VulkanBuffer* buffer = nullptr;
    VulkanImage* image = nullptr;
};

// Track for resource transitions
// Necessary parameters for VkImageMemoryBarrier 
struct RDGResourceState
{
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags accessMask = 0;
    VkPipelineStageFlags stageMask = 0;
};
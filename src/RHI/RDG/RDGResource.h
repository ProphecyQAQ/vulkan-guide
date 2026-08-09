#pragma once
#include <string>
#include <variant>

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

// Track for resource transitions
// Necessary parameters for VkImageMemoryBarrier 
struct RDGResourceState
{
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags2 accessMask = 0;
    VkPipelineStageFlags2 stageMask = 0;
};

struct RDGResource
{
    uint32_t handle = 0;

    std::string name;
    enum RDGResourceType type = RDGResourceType::NONE;
    bool isExternal = false;

    // resource desc
    std::variant<RDGBufferDesc, RDGTextureDesc> desc;

    // runtime resource
    RDGResourceState currentState;
    VulkanBuffer* buffer = nullptr;
    VulkanImage* image = nullptr;
};

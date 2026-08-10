#pragma once
#include <Vulkan/VulkanPCH.h>
#include <Vulkan/VulkanBuffer.h>
#include <Vulkan/VulkanImage.h>
#include <Vulkan/VulkanDevice.h>

#include <RDG/RDGResource.h>

class RDGPassContext
{
public:
    RDGPassContext(VulkanDevice* device, std::vector<RDGResource>& resources) : device(device), resources(resources) {}

    VulkanDevice* getDevice() const;
    VkExtent3D getImageExtent(uint32_t resourceHandle) const;

    VulkanImage* getImage(uint32_t resourceHandle) const;
    VulkanBuffer* getBuffer(uint32_t resourceHandle) const;

    VkRenderingAttachmentInfo getColorAttachment(uint32_t resourceHandle, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD);
    VkRenderingAttachmentInfo getDepthAttachment(uint32_t resourceHandle, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD);
private:
    VulkanDevice* device;
    std::vector<RDGResource>& resources;
};
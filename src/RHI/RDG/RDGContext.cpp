#include <RDG/RDGContext.h>

VulkanDevice* RDGPassContext::getDevice() const
{
    return device;
}

VkExtent3D RDGPassContext::getImageExtent(uint32_t resourceHandle) const
{
    RDGResource& resource = resources[resourceHandle];
    assert(resource.image != nullptr);
    return resource.image->getExtent();
}

VulkanImage* RDGPassContext::getImage(uint32_t resourceHandle) const
{
    RDGResource& resource = resources[resourceHandle];
    assert(resource.image != nullptr);
    return resource.image;
}

VulkanBuffer* RDGPassContext::getBuffer(uint32_t resourceHandle) const
{
    RDGResource& resource = resources[resourceHandle];
    assert(resource.buffer != nullptr);
    return resource.buffer;
}

VkRenderingAttachmentInfo RDGPassContext::getColorAttachment(uint32_t resourceHandle, VkAttachmentLoadOp loadOp)
{
    return VkRenderingAttachmentInfo{
        .imageView = getImage(resourceHandle)->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = loadOp,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
}

VkRenderingAttachmentInfo RDGPassContext::getDepthAttachment(uint32_t resourceHandle, VkAttachmentLoadOp loadOp)
{
    return VkRenderingAttachmentInfo{
        .imageView = getImage(resourceHandle)->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = loadOp,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
}
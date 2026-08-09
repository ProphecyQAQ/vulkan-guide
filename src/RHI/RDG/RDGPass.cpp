#include <RDG/RDGPass.h>

RDGPass& RDGPass::write(uint32_t resourceHandle, VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask, VkImageLayout layout)
{
    RDGPassAccess access;
    access.resourceHandle = resourceHandle;
    access.stageMask = stageMask;
    access.accessMask = accessMask;
    access.layout = layout;
    access.isWrite = true;

    resourceAccesses.push_back(access);
    return *this;
}

RDGPass& RDGPass::read(uint32_t resourceHandle, VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask, VkImageLayout layout)
{
    RDGPassAccess access;
    access.resourceHandle = resourceHandle;
    access.stageMask = stageMask;
    access.accessMask = accessMask;
    access.layout = layout;
    access.isWrite = false;

    resourceAccesses.push_back(access);
    return *this;
}

RDGPass& RDGPass::colorAttachment(uint32_t resourceHandle, uint32_t slot, VkAttachmentLoadOp loadOp)
{
    RDGPassAccess access;
    access.resourceHandle = resourceHandle;
    access.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    access.accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    access.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    access.isWrite = true;

    resourceAccesses.push_back(access);
    return *this;
}

RDGPass& RDGPass::depthAttachment(uint32_t resourceHandle, VkAttachmentLoadOp loadOp)
{
    RDGPassAccess access;
    access.resourceHandle = resourceHandle;
    access.stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    access.accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    access.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    access.isWrite = true;

    resourceAccesses.push_back(access);
    return *this;
}
#pragma once
#include <string>

#include <Vulkan/VulkanPCH.h>
#include <RDG/RDGContext.h>

struct RDGPassAccess
{
    uint32_t resourceHandle = 0;

    VkPipelineStageFlags2 stageMask = 0;
    VkAccessFlags2 accessMask = 0;
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    std::optional<uint32_t> slot;

    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    bool isWrite = false;
};

using RDGExecutionFunc = std::function<void(VkCommandBuffer cmd, RDGPassContext& ctx)>;

class RDGPass
{
public:
    RDGPass(const std::string& name) : name(name) {}

    const std::string& GetName() const { return name; }

    const std::vector<RDGPassAccess>& GetResourceAccesses() const
    {
        return resourceAccesses;
    }

    const std::vector<uint32_t>& GetPredecessors() const
    {
        return predecessors;
    }

    const std::vector<uint32_t>& GetSuccessors() const
    {
        return successors;
    }
    void addSuccessor(uint32_t successorId) 
    {
        successors.push_back(successorId);
    }

    uint32_t getIndex() const { return index; }
    void setIndex(uint32_t idx) { index = idx; }

    bool isExecutionValid() const { return executionFunc != nullptr; }
    void execute(VkCommandBuffer cmd, RDGPassContext& ctx) { executionFunc(cmd, ctx); }

public:
    // resource access
    RDGPass& write(uint32_t resourceHandle, VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask, VkImageLayout layout);
    RDGPass& read(uint32_t resourceHandle, VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask, VkImageLayout layout);

    RDGPass& colorAttachment(uint32_t resourceHandle, uint32_t slot, VkAttachmentLoadOp loadOp);
    RDGPass& depthAttachment(uint32_t resourceHandle, VkAttachmentLoadOp loadOp);

    RDGPass& setExecution(RDGExecutionFunc func)
    {
        executionFunc = func;
        return *this;
    }

private:
    std::string name;
    std::vector<RDGPassAccess> resourceAccesses;

    // Ruintime info
    uint32_t index = 0;
    std::vector<uint32_t> predecessors;
    std::vector<uint32_t> successors;

    RDGExecutionFunc executionFunc;
};
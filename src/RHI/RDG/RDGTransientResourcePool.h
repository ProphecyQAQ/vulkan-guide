#pragma once

#include <RDG/RDGResource.h>

#include <Vulkan/VulkanImage.h>
#include <Vulkan/VulkanBuffer.h>

class RDGTransientResourcePool
{
public:
    static RDGTransientResourcePool& get();

    RDGTransientResourcePool() = default;
    ~RDGTransientResourcePool();
    
    VulkanImage* acquireImage(const RDGTextureDesc& desc);
    VulkanBuffer* acquireBuffer(const RDGBufferDesc& desc);

    void releaseAll();

private:
    std::vector<VulkanImage*>  images;
    std::vector<VulkanBuffer*> buffers;
};
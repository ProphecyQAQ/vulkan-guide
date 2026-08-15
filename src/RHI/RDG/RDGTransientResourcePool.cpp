#include <RDG/RDGTransientResourcePool.h>

RDGTransientResourcePool& RDGTransientResourcePool::get()
{
    static RDGTransientResourcePool instance;
    return instance;
}

RDGTransientResourcePool::~RDGTransientResourcePool()
{
    releaseAll();
}

VulkanImage* RDGTransientResourcePool::acquireImage(const RDGTextureDesc& desc)
{
    VulkanImage* image = new VulkanImage(desc.extent, desc.format, desc.usage, desc.mipmapped);
    images.push_back(image);

    return image;
}

VulkanBuffer* RDGTransientResourcePool::acquireBuffer(const RDGBufferDesc& desc)
{
    VulkanBuffer* buffer = new VulkanBuffer(desc.size, desc.usage, desc.memUsage);
    buffers.push_back(buffer);

    return buffer;
}

void RDGTransientResourcePool::releaseAll()
{
    for (VulkanImage* image : images)
    {
        delete image;
    }

    for (VulkanBuffer* buffer : buffers)
    {
        delete buffer;
    }

    images.clear();
    buffers.clear();
}
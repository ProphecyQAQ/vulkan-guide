#include <RDG/RDGGraph.h>

RDGPass& RDGGraph::addPass(std::string passName)
{
    passes.emplace_back(passName);
    return passes.back();
}

uint32_t RDGGraph::createTexture(const std::string& name, const RDGTextureDesc& desc)
{
    RDGResource resource;
    resource.handle = static_cast<uint32_t>(resources.size());
    resource.name = name;
    resource.type = RDG_RESOURCE_TYPE_TEXTURE;
    resource.desc = desc;

    resources.push_back(resource);
    return resource.handle;
}

uint32_t RDGGraph::createBuffer(const std::string& name, const RDGBufferDesc& desc)
{
    RDGResource resource;
    resource.handle = static_cast<uint32_t>(resources.size());
    resource.name = name;
    resource.type = RDG_RESOURCE_TYPE_BUFFER;
    resource.desc = desc;

    resources.push_back(resource);
    return resource.handle;
}

uint32_t RDGGraph::importTexture(const std::string& name, VulkanImage* image)
{
    RDGResource resource;
    resource.handle = static_cast<uint32_t>(resources.size());
    resource.name = name;
    resource.type = RDG_RESOURCE_TYPE_TEXTURE;
    resource.isExternal = true;
    resource.image = image;

    resources.push_back(resource);
    return resource.handle;
}
#pragma once
#include <string>
#include <vector>

#include <RDG/RDGResource.h>
#include <RDG/RDGPass.h>

class RDGGraph
{
public:
    RDGGraph(std::string name) : name(name) {};
    ~RDGGraph() {};

    std::string getName() const { return name; }

    RDGPass& addPass(std::string passName);

    uint32_t createTexture(const std::string& name, const RDGTextureDesc& desc);
    uint32_t createBuffer(const std::string& name, const RDGBufferDesc& desc);
    uint32_t importTexture(const std::string& name, VulkanImage* image);
private:
    std::string name;

    std::vector<RDGResource> resources;
    std::vector<RDGPass> passes;
};
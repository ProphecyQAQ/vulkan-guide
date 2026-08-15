#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>

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

    void orderBefore(std::string& before, std::string& after);

public:
    void compile();

private:
    const std::string& getPassNameById(uint32_t idx) const;
private:
    std::string name;
    bool isCompiled = false;

    std::vector<RDGResource> resources;
    std::vector<RDGPass> passes;

    // Use orderBefore to display the specified pass order.
    std::map<std::string, std::set<std::string>> explicitOrder;

    // compile result
    std::vector<uint32_t> executeOrder;
};
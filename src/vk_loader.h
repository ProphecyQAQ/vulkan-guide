#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>

#include <unordered_map>
#include <filesystem>

class VulkanEngine;

struct GLTFMaterial {
	MaterialInstance data;
};

struct Bounds
{
    glm::vec3 origin;
    float sphereRadius;
    glm::vec3 extents;

    Bounds() = default;

    Bounds(glm::vec3 minPos, glm::vec3 maxPos)
    {
        origin = (minPos + maxPos) / 2.f;
        extents = (maxPos - minPos) / 2.f;
        sphereRadius = glm::length(extents);
    }
};

struct GeoSurface
{
    uint32_t startIndex;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset
{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

struct LoadedGLTF : public IRenderable {

    // store all data 
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    std::vector<std::shared_ptr<Node>> parentNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    AllocatedBuffer materialDataBuffer;

    VulkanEngine *engine;

    ~LoadedGLTF() { clear();};

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx); 
private:
    void clear();
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath);
std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine, std::filesystem::path filePath);
// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <limits>
#include <algorithm>
#include <set>
#include <functional>
#include <deque>
#include <stdexcept>
#include <chrono>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <magic_enum.hpp>

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            fmt::println("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

struct QueueFamilyIndices 
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool is_complete(){
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
};

struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

// holds the resources needed for a mesh
struct GPUMeshBuffers {
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

// push constants for our mesh object draws
struct GPUDrawPushConstant {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

// Scene Data
struct GPUSceneData{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;
};

//>material
enum class MaterialPass :uint8_t {
    MainColor,
    Transparent,
    Other
};
struct MaterialPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
    MaterialPass passType;
};
//>material

//> renderable

struct DrawContext;

// bass class for a renderable dynamic object
class IRenderable {

    virtual void Draw(const glm::mat4& parentMatrix, DrawContext& ctx) = 0;
};


// implement of a drawable scene node
// contains a list of children and a transform
struct Node : public IRenderable {

    glm::mat4 localTransform;
    glm::mat4 worldTransform;

    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    void refreshTransform(const glm::mat4& parentMatrix)
    {
        worldTransform = parentMatrix * localTransform;
        for (auto child : children)
        {
            child->refreshTransform(worldTransform);
        }
    }

    virtual void Draw(const glm::mat4& parentMatrix, DrawContext& ctx) override {
        for (auto child : children)
        {
            child->Draw(parentMatrix, ctx);
        }
    }
};
//> renderable
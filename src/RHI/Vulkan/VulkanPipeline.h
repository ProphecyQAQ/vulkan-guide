#pragma once

#include <Vulkan/VulkanPCH.h>
#include <Vulkan/VulkanDescriptorSet.h>

class VulkanPipeline;

enum VulkanShaderStage
{
    VK_SHADER_VERTEX = 0,
    VK_SHADER_FRAGMENT = 1,
    VK_SHADER_COMPUTE = 2,

    VK_SHADER_STAGE_COUNT,
};

class VulkanLayout
{
public:
    VulkanLayout(VulkanDevice& device);
    ~VulkanLayout();

    // get descriptor set layout
    VulkanDescriptorSetLayout::Builder& getDescriptorSetLayoutBuilder() { return discriptorLayoutBuilder; }
    void addDescriptorSetLayout(VkDescriptorSetLayout layout) { descriptorSetLayouts.push_back(layout); }

    // set push constant
    void setPushConstantType(uint32_t size, VkShaderStageFlags shaderStage, uint32_t offset = 0);

    // set pipeline create info
    void setInputAssembly(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable = VK_FALSE);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void setPolygonMode(VkPolygonMode mode);
    // not supprort msaa yet
    void setMultiSampleState(uint32_t sampleNum);
    void setShaderModule(VkShaderModule shader, VulkanShaderStage stage);
    void setColorAttachmentFormat(VkFormat format);

    void setDepthFormat(VkFormat format);
    void enableDepthTest(bool depthWriteEnable, VkCompareOp op);
    void disableDepthTest();

    void disableBlend();
    void setBlendAdditive();
    void setBlendAlpha();

    VulkanPipeline createPipeline();
private:
    std::vector<VkPipelineShaderStageCreateInfo> getShaderStageCreateInfo();
private:
    VulkanDevice& device;

    VulkanDescriptorSetLayout::Builder discriptorLayoutBuilder;
    VulkanDescriptorSetLayout* descriptorLayout;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    // push constant
    VkPushConstantRange pushConstantRange;

    // shader path
    VkShaderModule shaderModules[VK_SHADER_STAGE_COUNT];

    // pipeline layout info
    VkPipelineLayout pipelineLayout;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampleState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    VkPipelineRenderingCreateInfo renderInfo;
};

class VulkanPipeline
{
public:
    VulkanPipeline(VulkanDevice& device, VulkanLayout layout, VkPipeline pipeline);
    ~VulkanPipeline();
private:
    VulkanDevice& device;
    VulkanLayout layout;
    VkPipeline pipeline;
};

class VulkanComputePipeline
{
public:
    VulkanComputePipeline(VulkanDevice& device, VkShaderModule computeShader);
    ~VulkanComputePipeline();
private:
    VulkanDevice& device;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};
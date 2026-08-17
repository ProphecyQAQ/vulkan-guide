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

enum VulkanPipelineType
{
    VK_GRAPHICS_PIPELINE = 1,
    VK_COMPUTE_PIPELINE = 2
};

class VulkanLayout
{
public:
    VulkanLayout(VulkanDevice& device);
    ~VulkanLayout();

    // get descriptor set layout
    VulkanDescriptorSetLayout::Builder& getDescriptorSetLayoutBuilder() { return discriptorLayoutBuilder; }

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

    VulkanPipeline* createPipeline();
    VulkanPipeline* createComputePipeline();
public:
    VulkanDescriptorSetLayout* getDescriptorSetLayout() const { return descriptorSetLayout; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
private:
    std::vector<VkPipelineShaderStageCreateInfo> getShaderStageCreateInfo();
private:
    VulkanDevice& device;

    VulkanDescriptorSetLayout::Builder discriptorLayoutBuilder;
    VulkanDescriptorSetLayout* commonDescriptorLayout;
    VulkanDescriptorSetLayout* descriptorSetLayout;

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
    VkFormat colorAttachmentFormat;
};

class VulkanPipeline
{
public:
    VulkanPipeline(VulkanDevice& device, VulkanLayout* layout, VkPipeline pipeline, VulkanPipelineType type);
    ~VulkanPipeline();

    VkPipeline getPipeline() const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return layout->getPipelineLayout(); }
    VulkanDescriptorSetLayout* getDescriptorSetLayout() const { return layout->getDescriptorSetLayout(); }
private:
    VulkanPipelineType type;

    VulkanDevice& device;
    VulkanLayout* layout;
    VkPipeline pipeline;
};

class VulkanPipelineLibrary
{
public:
    VulkanPipelineLibrary() = default;
    ~VulkanPipelineLibrary();
    
    bool addPipelines(std::string name, VulkanPipeline* pipeline);
    VulkanPipeline* getPipelines(std::string name);
private:
    std::map<std::string, VulkanPipeline*> pipelineMap;
};
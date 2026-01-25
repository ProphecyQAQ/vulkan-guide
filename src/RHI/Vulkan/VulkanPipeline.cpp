#include <Vulkan/VulkanPipeline.h>
#include <Vulkan/VulkanHelper.h>

// ----------------- VulkanLayout ------------------
VulkanLayout::VulkanLayout(VulkanDevice& device)
    : device(device), descriptorSetLayout(nullptr), commonDescriptorLayout(nullptr)
{
    for (VkShaderModule& module : shaderModules)
    {
        module = VK_NULL_HANDLE;
    }
}

VulkanLayout::~VulkanLayout()
{
    if (descriptorSetLayout)
    {
        delete descriptorSetLayout;
    }

    if (commonDescriptorLayout)
    {
        delete commonDescriptorLayout;
    }

    for (VkShaderModule shader : shaderModules)
    {
        if (shader != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(device.getDevice(), shader, nullptr);
        }
    }

    vkDestroyPipelineLayout(device.getDevice(), pipelineLayout, nullptr);
}

void VulkanLayout::setPushConstantType(uint32_t size, VkShaderStageFlags shaderStage, uint32_t offset /*= 0*/)
{
    pushConstantRange.size = size;
    pushConstantRange.stageFlags = shaderStage;
    pushConstantRange.offset = 0;
}

void VulkanLayout::setInputAssembly(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
{
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = primitiveRestartEnable;
}

void VulkanLayout::setShaderModule(VkShaderModule shader, VulkanShaderStage stage)
{
    assert(shaderModules[stage] == VK_NULL_HANDLE);
    shaderModules[stage] = shader;
}

void VulkanLayout::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    rasterizationState.cullMode = cullMode;
    rasterizationState.frontFace = frontFace;
}

void VulkanLayout::setPolygonMode(VkPolygonMode mode)
{
    rasterizationState.polygonMode = mode;
    rasterizationState.lineWidth = 1.0f;
}

void VulkanLayout::setMultiSampleState(uint32_t sampleNum)
{
    assert(sampleNum == 1);
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.minSampleShading = 1.0f;
    multisampleState.pSampleMask = nullptr;
    // no alpha to coverage either
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;
}

void VulkanLayout::setColorAttachmentFormat(VkFormat format)
{
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachmentFormats = &format;
}

void VulkanLayout::disableBlend()
{
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}

void VulkanLayout::setBlendAdditive()
{
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}

void VulkanLayout::setBlendAlpha()
{
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}

void VulkanLayout::setDepthFormat(VkFormat format)
{
    renderInfo.depthAttachmentFormat = format;
}

void VulkanLayout::enableDepthTest(bool depthWriteEnable, VkCompareOp op)
{
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencilState.depthCompareOp = op;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = {};
    depthStencilState.back = {};
    depthStencilState.minDepthBounds = 0.f;
    depthStencilState.maxDepthBounds = 1.f;
}

void VulkanLayout::disableDepthTest()
{
    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_NEVER;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = {};
    depthStencilState.back = {};
    depthStencilState.minDepthBounds = 0.f;
    depthStencilState.maxDepthBounds = 1.f;
}

std::vector<VkPipelineShaderStageCreateInfo> VulkanLayout::getShaderStageCreateInfo()
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (int i = 0; i < VK_SHADER_STAGE_COUNT; ++i)
    {
        if (shaderModules[i] != VK_NULL_HANDLE)
        {
            VkShaderStageFlagBits stageFlag;
            switch (i)
            {
            case VK_SHADER_VERTEX:
                stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case VK_SHADER_FRAGMENT:
                stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case VK_SHADER_COMPUTE:
                stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                assert(false && "unsupported shader stage");
                continue;
            }
            shaderStages.push_back(VulkanHeaplerLibrary::pipelineShaderStageCreateInfo(stageFlag, shaderModules[i]));
        }
    }
    return shaderStages;
}

VulkanPipeline VulkanLayout::createPipeline()
{
    // build descriptor set layout
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    descriptorSetLayout = discriptorLayoutBuilder.build(device.getDevice(), VK_SHADER_STAGE_ALL);
    descriptorSetLayouts.push_back(descriptorSetLayout->getDescriptorSetLayout());
    if (commonDescriptorLayout && commonDescriptorLayout->getDescriptorSetLayout() != VK_NULL_HANDLE)
    {
        descriptorSetLayouts.push_back(commonDescriptorLayout->getDescriptorSetLayout());
    }

    // craete pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanHeaplerLibrary::pipelineLayoutCreateInfo();
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.setLayoutCount = sizeof(descriptorSetLayouts);

    VK_CHECK(vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

    // make viewport state from our viewport and scissor
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // set dump color blend
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;

    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // completely clear VertexInputStateCreateInfo, as we have no need for it
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    // build graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderInfo;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = getShaderStageCreateInfo();

    pipelineInfo.stageCount = (uint32_t)shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.layout = pipelineLayout;    

    // set dynamic state
    VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = state;

    pipelineInfo.pDynamicState = &dynamicState;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        LOG_ERROR("[PipelineBuilder] [build_pipeline] failed to create pipeline");
        assert(0);
    }

    return VulkanPipeline(device, *this, pipeline);
}

VulkanComputePipeline* VulkanLayout::createComputePipeline()
{
    // build descriptor set layout
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    descriptorSetLayout = discriptorLayoutBuilder.build(device.getDevice(), VK_SHADER_STAGE_ALL);
    descriptorSetLayouts.push_back(descriptorSetLayout->getDescriptorSetLayout());
    if (commonDescriptorLayout && commonDescriptorLayout->getDescriptorSetLayout() != VK_NULL_HANDLE)
    {
        descriptorSetLayouts.push_back(commonDescriptorLayout->getDescriptorSetLayout());
    }

    // craete pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanHeaplerLibrary::pipelineLayoutCreateInfo();
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();

    VK_CHECK(vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

    // build compute pipeline
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = getShaderStageCreateInfo();
    assert(shaderStages.size() == 1 && "compute pipeline should only have one compute shader stage");
    pipelineInfo.stage = shaderStages[0];

    VkPipeline pipeline;
    if (vkCreateComputePipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        LOG_ERROR("failed to create compute pipeline");
        assert(0);
    }

    return new VulkanComputePipeline(device, *this, pipeline);
}
// ----------------- VulkanLayout ------------------

// ----------------- VulkanPipelin ------------------
VulkanPipeline::VulkanPipeline(VulkanDevice& device, VulkanLayout layout, VkPipeline pipeline)
    : device(device), layout(layout), pipeline(pipeline)
{
}

VulkanPipeline::~VulkanPipeline()
{
    vkDestroyPipeline(device.getDevice(), pipeline, nullptr);
}
// ----------------- VulkanPipelin ------------------

// ----------------- VulkanComputePipeline ------------------
VulkanComputePipeline::VulkanComputePipeline(VulkanDevice& device, VulkanLayout layout, VkPipeline pipeline)
    : device(device), layout(layout), pipeline(pipeline)
{}

VulkanComputePipeline::~VulkanComputePipeline()
{
    vkDestroyPipeline(device.getDevice(), pipeline, nullptr);
}
// ----------------- VulkanComputePipeline ------------------
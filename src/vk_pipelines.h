#pragma once 
#include <vk_types.h>
#include <vk_pipelines.h>
#include <fstream>
#include <vk_initializers.h>

namespace vkutil {

    bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);

};

class PipelineBuilder 
{
private:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineLayout _pipelineLayout;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentformat;
public:
    PipelineBuilder() {  clear(); }

    void clear();

    VkPipeline build_pipeline(VkDevice device);
    
    void set_pipeline_layout(VkPipelineLayout layout);
    void set_shader(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void set_input_topology(VkPrimitiveTopology topology);
    void set_polygon_mode(VkPolygonMode mode);
    void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void set_multisampling_none();

    void disable_blending();
    void enable_blend_additive();
    void enable_blend_alphablend();

    void set_color_attachment_format(VkFormat format);
    void set_depth_format(VkFormat format);

    void enable_depthtest(bool depthWriteEnable, VkCompareOp op);
    void disable_depthtest();
};
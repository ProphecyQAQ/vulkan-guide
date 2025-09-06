#include <vk_material.h>
#include <vk_pipelines.h>
#include <vk_engine.h>

void GLTFMetallic_Roughness::build_pipelines(VulkanEngine *engine)
{
    VkShaderModule meshFragShader;
	if (!vkutil::load_shader_module("../../shaders/mesh.frag.spv", engine->get_device(), &meshFragShader)) {
		fmt::println("Error when building the triangle fragment shader module");
	}

	VkShaderModule meshVertexShader;
	if (!vkutil::load_shader_module("../../shaders/mesh.vert.spv", engine->get_device(), &meshVertexShader)) {
		fmt::println("Error when building the triangle vertex shader module");
	}

    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstant);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    materialLayout = layoutBuilder.build(engine->get_device(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout layouts[] = {
        engine->get_gpu_scene_data_descriptor_layout(),
        materialLayout
    };
    VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
    mesh_layout_info.setLayoutCount = 2;
    mesh_layout_info.pSetLayouts = layouts;
    mesh_layout_info.pushConstantRangeCount = 1;
    mesh_layout_info.pPushConstantRanges = &matrixRange;

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(engine->get_device(), &mesh_layout_info, nullptr, &layout));

    opaquePipeline.layout = layout;
    transparentPipeline.layout = layout;

    // build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_pipeline_layout(layout);
    pipelineBuilder.set_shader(meshVertexShader, meshFragShader);
    pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.disable_blending();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // render format
    pipelineBuilder.set_color_attachment_format(engine->_drawImage.imageFormat);
    pipelineBuilder.set_depth_format(engine->_depthImage.imageFormat);

    // build pipeline
    opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->get_device());

    pipelineBuilder.enable_blend_additive();
	pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
    transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->get_device());

    vkDestroyShaderModule(engine->get_device(), meshFragShader, nullptr);
    vkDestroyShaderModule(engine->get_device(), meshVertexShader, nullptr);
}

MaterialInstance GLTFMetallic_Roughness::write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
{
    MaterialInstance materialInstance;
    materialInstance.passType = pass;

    if (pass == MaterialPass::Transparent)
    {
        materialInstance.pipeline = &transparentPipeline;
    }
    else
    {
        materialInstance.pipeline = &opaquePipeline;
    }

    materialInstance.materialSet = descriptorAllocator.allocate(device, materialLayout, nullptr);

    writer.clear();
    writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.write_image(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.write_image(1, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    writer.update_set(device, materialInstance.materialSet);

    return materialInstance;
}

void GLTFMetallic_Roughness::clear_resources(VkDevice device)
{
    vkDestroyPipelineLayout(device, opaquePipeline.layout, nullptr);
    vkDestroyPipeline(device, opaquePipeline.pipeline, nullptr);
    vkDestroyPipeline(device, transparentPipeline.pipeline, nullptr);

    vkDestroyDescriptorSetLayout(device, materialLayout, nullptr);
}
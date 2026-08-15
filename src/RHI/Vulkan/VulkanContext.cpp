#include <vector>
#include <Core/Log.h>
#include <Vulkan/VulkanContext.h>
#include <Vulkan/VulkanHelper.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <RHI/RDG/RDGGraph.h>
#include <RHI/RDG/RDGResource.h>

// ------------------------------ FrameContext -----------------------
FrameContext::FrameContext(VulkanDevice& device)
    : device(device)
{
    commandBufferPool = new VulkanCommandBufferPool(device, VulkanCommandBufferType::VK_CMD_BUFFER_TYPE_PRIMARY);
    commandBuffer = commandBufferPool->create();

    frameDescriptorPoolSet = new VulkanDescriptorPoolSet(device);

    VkFenceCreateInfo fenceInfo = VulkanHeaplerLibrary::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &renderFence));

    VkSemaphoreCreateInfo semaphoreInfo = VulkanHeaplerLibrary::semaphoreCreateInfo();
    VK_CHECK(vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &swapchainSemaphore));
    VK_CHECK(vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &renderSemaphore));
}

FrameContext::~FrameContext()
{
    delete commandBufferPool;
    delete frameDescriptorPoolSet;
    vkDestroyFence(device.getDevice(), renderFence, nullptr);

    vkDestroySemaphore(device.getDevice(), swapchainSemaphore, nullptr);
    vkDestroySemaphore(device.getDevice(), renderSemaphore, nullptr);
}
// ------------------------------ FrameContext -----------------------

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

static VulkanContext* ctx = nullptr;
VulkanContext* VulkanContext::get()
{
    return ctx;
}

VulkanContext::~VulkanContext()
{
    // delete images that use VMA before destroying allocator
    delete renderImage;
    delete depthImage;

    // clear render datas that contain VMA buffers
    frameRenderDatas.clear();

    vmaDestroyAllocator(allocator);

    delete swapChain;

    // clean immediate ctx
    vkDestroyFence(vulkanDevice->getDevice(), immediateFence, nullptr);
    delete immediateCmdPool;

    // clean frame ctx  
    for (FrameContext* ctx : frameContexts)
    {
        delete ctx;
    }

    // delete global descriptor pool set
    delete globalDescriptorPoolSet;

    // delete vulkan device
    delete vulkanDevice;
    
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void VulkanContext::init(Window* window)
{
    ctx = this;

    deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
    };

    // create vkinstance create info
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    // get required extensions from windowing system
    std::vector<const char*> sdlExtestions = window->getVulkanExtensions();

    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(sdlExtestions.size());
    instanceInfo.ppEnabledExtensionNames = sdlExtestions.data();

    // create vkinstance
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }

    // create surface
    if (!SDL_Vulkan_CreateSurface(reinterpret_cast<SDL_Window*>(window->getNativeWindow()), instance, &surface))
    {
        throw std::runtime_error("[VulkanSwapChain] [init] create surface failed");
    }

    // create vulkan device
    vulkanDevice = new VulkanDevice(instance);

    // Init swapchain
    swapChain = new VulkanSwapChain(instance, vulkanDevice, surface, window, WIDTH, HEIGHT);

    // init vma allocator
    VmaAllocatorCreateInfo vmaCreateInfo{};
    vmaCreateInfo.physicalDevice = vulkanDevice->getPhysicalDevice();
    vmaCreateInfo.device = vulkanDevice->getDevice();
    vmaCreateInfo.instance = instance;
    vmaCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&vmaCreateInfo, &allocator);

    // Immdiate context init
    initImmediateCtx();
    
    // Frame context init
    initFrameContext();

    // global descriptor pool set
    globalDescriptorPoolSet = new VulkanDescriptorPoolSet(*vulkanDevice);

    // create render image
    renderImage = new VulkanImage(
        VkExtent3D{ WIDTH, HEIGHT, 1},
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        false
    );

    // create depth image
    depthImage = new VulkanImage(
        VkExtent3D{ WIDTH, HEIGHT, 1},
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        false
    );

    // blow is for test
    {
        initComputePipeline();

        initUnlitPipeline();
    }
}

void VulkanContext::initImmediateCtx()
{
    // create immediate command pool and buffer
    immediateCmdPool = new VulkanCommandBufferPool(*vulkanDevice, VulkanCommandBufferType::VK_CMD_BUFFER_TYPE_PRIMARY);
    immediateCmdBuffer = immediateCmdPool->create();

    VkFenceCreateInfo fenceInfo = VulkanHeaplerLibrary::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(vulkanDevice->getDevice(), &fenceInfo, nullptr, &immediateFence));
}

void VulkanContext::initFrameContext()
{
    for (int32_t idx = 0; idx < FRAME_OVERLAP; idx ++)
    {
        frameContexts.push_back(new FrameContext(*vulkanDevice));
    }
}

void VulkanContext::initComputePipeline()
{
    computePipelineLayout = new VulkanLayout(*vulkanDevice);
    computePipelineLayout->getDescriptorSetLayoutBuilder()
        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1);
    computePipelineLayout->setPushConstantType(sizeof(ComputePipelinePushConstantData), VK_SHADER_STAGE_COMPUTE_BIT);
    VkShaderModule computeShader;
    if (!VulkanHeaplerLibrary::loadShaderModule("../../shaders/sky.comp.spv", vulkanDevice->getDevice(), &computeShader)) {
        LOG_ERROR("Error when building the compute shader \n");
    }
    computePipelineLayout->setShaderModule(computeShader, VulkanShaderStage::VK_SHADER_COMPUTE);

    computePipeline = computePipelineLayout->createComputePipeline();
}

void VulkanContext::drawComputePipeline(VkCommandBuffer cmd)
{
    RDGGraph computeGraph("ComputeDemo");

    RDGTextureDesc rtDesc;
    rtDesc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    rtDesc.extent = {WIDTH, HEIGHT, 1};
    rtDesc.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    uint32_t renderHandle = computeGraph.createTexture("RenderImage", rtDesc);

    computeGraph.addPass("ComputeSky")
    .write(renderHandle, 
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
        VK_ACCESS_2_SHADER_WRITE_BIT, 
        VK_IMAGE_LAYOUT_GENERAL)
    .setExecution([this, renderHandle](VkCommandBuffer cmd, RDGPassContext& ctx){
        VulkanImage* renderRT = ctx.getImage(renderHandle);

        // make a clear-color
        VkClearColorValue clearValue;
        float flash = std::abs(std::sin(frameCount/ 10.f));
        clearValue = { {0.f, 0.f, flash, 1.f} };
        
        VkImageSubresourceRange clearRange = VulkanHeaplerLibrary::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        // clear image
        vkCmdClearColorImage(cmd, renderRT->getImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->getPipeline());

        // Allocate descriptor slot
        VkDescriptorSet ds = getCurrentFrameContext()->frameDescriptorPoolSet->allocateDescriptorSet(computePipelineLayout->getDescriptorSetLayout());
        VulkanDescriptorSet::Writer computeDescriptorSetWriter;
        computeDescriptorSetWriter.writeImage(0, renderRT->getImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        computeDescriptorSetWriter.update(*vulkanDevice, ds);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout->getPipelineLayout(), 0, 1, &ds, 0, nullptr);

        ComputePipelinePushConstantData.data1 = glm::vec4(0.1, 0.2, 0.4 ,0.97);

        vkCmdPushConstants(cmd, computePipelineLayout->getPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePipelinePushConstantData), &ComputePipelinePushConstantData);
        // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
        vkCmdDispatch(cmd, std::ceil(WIDTH / 16.0), std::ceil(HEIGHT / 16.0), 1);
    });

    computeGraph.addPass("DummyRead")
    .read(renderHandle,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
    .setExecution([this, renderHandle](VkCommandBuffer cmd, RDGPassContext& ctx){
        VulkanImage* renderRT = ctx.getImage(renderHandle);
        
        VkExtent2D dstSize = { renderImage->getExtent().width, renderImage->getExtent().height };
        VkExtent2D srcSize = { renderRT->getExtent().width, renderRT->getExtent().height };
        VulkanHeaplerLibrary::copyImageToImage(cmd, renderRT->getImage(), renderImage->getImage(), srcSize, dstSize);
    });
    
    computeGraph.compile();
    computeGraph.execute(cmd);
}

void VulkanContext::initUnlitPipeline()
{
    unlitPipelineLayout = new VulkanLayout(*vulkanDevice);

    unlitPipelineLayout->setPushConstantType(sizeof(UnlitPipelinePushConstantData), VK_SHADER_STAGE_VERTEX_BIT);
    unlitPipelineLayout->setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    unlitPipelineLayout->setPolygonMode(VK_POLYGON_MODE_FILL);
    unlitPipelineLayout->setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    unlitPipelineLayout->setMultiSampleState(1);
    unlitPipelineLayout->disableBlend();
    unlitPipelineLayout->enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    unlitPipelineLayout->setColorAttachmentFormat(renderImage->getFormat());
    unlitPipelineLayout->setDepthFormat(depthImage->getFormat());

    VkShaderModule vertShader;
    if (!VulkanHeaplerLibrary::loadShaderModule("../../shaders/colored_triangle_mesh.vert.spv", vulkanDevice->getDevice(), &vertShader)) {
        LOG_ERROR("Error when building the unlit vertex shader \n");
    }
    unlitPipelineLayout->setShaderModule(vertShader, VulkanShaderStage::VK_SHADER_VERTEX);
    VkShaderModule fragShader;
    if (!VulkanHeaplerLibrary::loadShaderModule("../../shaders/colored_triangle.frag.spv", vulkanDevice->getDevice(), &fragShader)) {
        LOG_ERROR("Error when building the unlit fragment shader \n");
    }
    unlitPipelineLayout->setShaderModule(fragShader, VulkanShaderStage::VK_SHADER_FRAGMENT);

    VulkanDescriptorSetLayout::Builder& descriptorSetLayoutBuilder = unlitPipelineLayout->getDescriptorSetLayoutBuilder();
    descriptorSetLayoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

    unlitPipeline = unlitPipelineLayout->createPipeline();
}

void VulkanContext::drawUnlitPipeline(VkCommandBuffer cmd)
{
    // connected to draw image
    VkRenderingAttachmentInfo colorAttachment = VulkanHeaplerLibrary::attachmentInfo(renderImage->getImageView(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depthAttachment = VulkanHeaplerLibrary::depthAttachmentInfo(depthImage->getImageView(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo renderInfo = VulkanHeaplerLibrary::renderingInfo(VkExtent2D{WIDTH, HEIGHT}, &colorAttachment, &depthAttachment);    
    vkCmdBeginRendering(cmd, &renderInfo);

    // set scene data
    RenderSystem* renderSystem = RenderSystem::get();
    sceneData.view = renderSystem->getScene()->getViewMatrix();
    sceneData.proj = glm::perspective(glm::radians(70.f),
     (float)WIDTH / (float)HEIGHT, 10000.f, 0.1f);
    sceneData.proj[1][1] *= -1;
    sceneData.viewproj = sceneData.proj * sceneData.view;

    SceneDataBuffer = new VulkanBuffer(
        sizeof(SceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    SceneDataBuffer->setData(&sceneData);

    VkDescriptorSet descriptorSet = getCurrentFrameContext()->frameDescriptorPoolSet->allocateDescriptorSet(unlitPipelineLayout->getDescriptorSetLayout());

    VulkanDescriptorSet::Writer sceneDataWriter;
    sceneDataWriter.writeBuffer(0, SceneDataBuffer->getBuffer(), sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    sceneDataWriter.update(*vulkanDevice, descriptorSet);

    for (RenderObject& renderObj : frameRenderDatas)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitPipeline->getPipeline());

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitPipelineLayout->getPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

        unlitPipelinePushConstantData.render_matrix = renderObj.transform; 
        unlitPipelinePushConstantData.vertexBufferAddress = renderObj.vertexBuffer->getBufferAddress();

        // set dynamic viewport and scissor
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(WIDTH);
        viewport.height = static_cast<float>(HEIGHT);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = WIDTH;
        scissor.extent.height = HEIGHT;

        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindIndexBuffer(cmd, renderObj.indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdPushConstants(cmd, unlitPipelineLayout->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UnlitPipelinePushConstantData), &unlitPipelinePushConstantData);

        vkCmdDrawIndexed(cmd, renderObj.indexCount, 1, renderObj.firstIndex, 0, 0);
    }

    vkCmdEndRendering(cmd);
}

void VulkanContext::beginFrame()
{
};

void VulkanContext::drawFrame()
{
    FrameContext* currentFrameCtx = getCurrentFrameContext();

    // wait last frame over
    VK_CHECK(vkWaitForFences(vulkanDevice->getDevice(), 1, &currentFrameCtx->renderFence, true, UINT64_MAX));
    currentFrameCtx->frameDescriptorPoolSet->clear();
    VK_CHECK(vkResetFences(vulkanDevice->getDevice(), 1, &currentFrameCtx->renderFence));

    // reset cmd
    VkCommandBuffer cmd = currentFrameCtx->commandBuffer->getHandle();
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // begin cmd recording
    VkCommandBufferBeginInfo cmdBeginInfo = VulkanHeaplerLibrary::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    uint32_t swapchainImageIndex;
	VkResult e = vkAcquireNextImageKHR(getVkDevice(), swapChain->getSwapChain(), 1000000000, currentFrameCtx->swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (e == VK_ERROR_OUT_OF_DATE_KHR) {     
		return ;
	}

    // test draw compute pipeline
    {   
        VulkanHeaplerLibrary::transitionImage(cmd, renderImage->getImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        drawComputePipeline(cmd);
    }

    // test draw unlit pipeline
    {
        VulkanHeaplerLibrary::transitionImage(cmd, renderImage->getImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        drawUnlitPipeline(cmd);
    }

    // transition the draw image and swapchain image into transfer layout
    VulkanHeaplerLibrary::transitionImage(cmd, renderImage->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VulkanHeaplerLibrary::transitionImage(cmd, swapChain->getImage(swapchainImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // copy draw image to swapchain image
    VulkanHeaplerLibrary::copyImageToImage(
        cmd, 
        renderImage->getImage(), 
        swapChain->getImage(swapchainImageIndex), 
        VkExtent2D{WIDTH, HEIGHT}, 
        VkExtent2D{WIDTH, HEIGHT}
    );

    VulkanHeaplerLibrary::transitionImage(cmd, swapChain->getImage(swapchainImageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // finish command buffer
    VK_CHECK(vkEndCommandBuffer(cmd));

    // prepare for submit to the queue
    // wait on the _presentSemaphore, as that semaphore is signaled when the swapchian is ready
    // will signal the _renderSemaphore, to signal that rendering has finished
    VkCommandBufferSubmitInfo cmdSubmitInfo = VulkanHeaplerLibrary::commandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitSemaInfo = VulkanHeaplerLibrary::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrameCtx->swapchainSemaphore);
    VkSemaphoreSubmitInfo signalSeamInfo = VulkanHeaplerLibrary::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrameCtx->renderSemaphore);
        
    VkSubmitInfo2 submitInfo = VulkanHeaplerLibrary::submitInfo(&cmdSubmitInfo, &signalSeamInfo, &waitSemaInfo);

    VK_CHECK(vkQueueSubmit2(vulkanDevice->getGraphicsQueue(), 1, &submitInfo, currentFrameCtx->renderFence));

    // prepare for present
    // put image we just rendered into window
    // need wait on _renderSemaphore
    VkSwapchainKHR swapChainHandle = swapChain->getSwapChain();
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pSwapchains = &swapChainHandle;
    presentInfo.swapchainCount = 1;
    presentInfo.pNext = nullptr;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &currentFrameCtx->renderSemaphore;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VkResult presentResult = vkQueuePresentKHR(vulkanDevice->getGraphicsQueue(), &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
    }

    frameCount ++;
};

void VulkanContext::endFrame()
{
    if (SceneDataBuffer)
    {
        delete SceneDataBuffer;
        SceneDataBuffer = nullptr;
    }
    frameRenderDatas.clear();
};

void VulkanContext::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func)
{
    // wait last frame over
    VK_CHECK(vkWaitForFences(vulkanDevice->getDevice(), 1, &immediateFence, true, UINT64_MAX));
    VK_CHECK(vkResetFences(vulkanDevice->getDevice(), 1, &immediateFence));

    VkCommandBuffer cmd = immediateCmdBuffer->getHandle();
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = VulkanHeaplerLibrary::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    func(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdSubmitInfo = VulkanHeaplerLibrary::commandBufferSubmitInfo(cmd);

    VkSubmitInfo2 submitInfo = VulkanHeaplerLibrary::submitInfo(&cmdSubmitInfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(vulkanDevice->getGraphicsQueue(), 1, &submitInfo, immediateFence));
    VK_CHECK(vkWaitForFences(vulkanDevice->getDevice(), 1, &immediateFence, true, UINT64_MAX));
}

void VulkanContext::submit(RenderData& renderData)
{
    RenderObject& renderObj = frameRenderDatas.emplace_back();

    size_t vertexBufferSize = renderData.vertices->size() * sizeof(Vertex);
    size_t indexBufferSize = renderData.indices->size() * sizeof(uint32_t);

    renderObj.firstIndex = 0;
    renderObj.indexCount = static_cast<uint32_t>(renderData.indices->size());
    renderObj.transform = renderData.transform;

    renderObj.vertexBuffer = new VulkanBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    renderObj.indexBuffer = new VulkanBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    
    VulkanBuffer stagingBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    // Copy vertex and index data to staging buffer
    void* data = stagingBuffer.getMappedData();
    memcpy(data, renderData.vertices->data(), vertexBufferSize);
    memcpy(static_cast<uint8_t*>(data) + vertexBufferSize, renderData.indices->data(), indexBufferSize);

    immediateSubmit([&](VkCommandBuffer cmd){
        VkBufferCopy vertexCopy{};
        vertexCopy.srcOffset = 0;
        vertexCopy.dstOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, stagingBuffer.getBuffer(), renderObj.vertexBuffer->getBuffer(), 1, &vertexCopy);

        VkBufferCopy indexCopy{};
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.dstOffset = 0;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, stagingBuffer.getBuffer(), renderObj.indexBuffer->getBuffer(), 1, &indexCopy);
    }
    );
}
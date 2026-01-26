#include <vector>
#include <Core/Log.h>
#include <Vulkan/VulkanContext.h>
#include <Vulkan/VulkanHelper.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

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

VulkanContext* ctx = nullptr;
VulkanContext* VulkanContext::get()
{
    return ctx;
}

VulkanContext::~VulkanContext()
{
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

    // delete image
    delete renderImage;

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
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

    // blow is for test
    {
        initComputePipeline();
    }
}

void VulkanContext::initImmediateCtx()
{
    // create immediate command pool and buffer
    immediateCmdPool = new VulkanCommandBufferPool(*vulkanDevice, VulkanCommandBufferType::VK_CMD_BUFFER_TYPE_PRIMARY);
    immediateCmdPool->create();

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
        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);
    computePipelineLayout->setPushConstantType(sizeof(ComputePipelinePushConstantData), VK_SHADER_STAGE_COMPUTE_BIT);
    VkShaderModule computeShader;
    if (!VulkanHeaplerLibrary::loadShaderModule("../../shaders/sky.comp.spv", vulkanDevice->getDevice(), &computeShader)) {
        LOG_ERROR("Error when building the compute shader \n");
    }
    computePipelineLayout->setShaderModule(computeShader, VulkanShaderStage::VK_SHADER_COMPUTE);

    computePipeline = computePipelineLayout->createComputePipeline();

    computePipelineDescriptorSet = globalDescriptorPoolSet->allocateDescriptorSet(computePipelineLayout->getDescriptorSetLayout());

    VulkanDescriptorSet::Writer computeDescriptorSetWriter;
    computeDescriptorSetWriter.writeImage(0, renderImage->getImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    computeDescriptorSetWriter.update(*vulkanDevice, computePipelineDescriptorSet);

    //default sky parameters
    ComputePipelinePushConstantData.data1 = glm::vec4(0.1, 0.2, 0.4 ,0.97);
}

void VulkanContext::drawComputePipeline(VkCommandBuffer cmd)
{
    // make a clear-color
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(frameCount/ 10.f));
    clearValue = { {0.f, 0.f, flash, 1.f} };

    VkImageSubresourceRange clearRange = VulkanHeaplerLibrary::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    // clear image
    vkCmdClearColorImage(cmd, renderImage->getImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);    

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->getPipeline());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout->getPipelineLayout(), 0, 1, &computePipelineDescriptorSet, 0, nullptr);

	vkCmdPushConstants(cmd, computePipelineLayout->getPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePipelinePushConstantData), &ComputePipelinePushConstantData);
	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(WIDTH / 16.0), std::ceil(HEIGHT / 16.0), 1);
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

    // transition the draw image and swapchain image into transfer layout
    VulkanHeaplerLibrary::transitionImage(cmd, renderImage->getImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VulkanHeaplerLibrary::transitionImage(cmd, swapChain->getImage(swapchainImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // copy draw image to swapchain image
    VulkanHeaplerLibrary::copyImageToImage(
        cmd, 
        renderImage->getImage(), 
        swapChain->getImage(swapchainImageIndex), 
        VkExtent2D{WIDTH, HEIGHT}, 
        VkExtent2D{WIDTH, HEIGHT});


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
{};
#pragma once

#include <Core/Window.h>
#include <RHI/GraphicsContext.h>
#include <Vulkan/VulkanPCH.h>
#include <Vulkan/VulkanSwapChain.h>
#include <Vulkan/VulkanDevice.h>
#include <Vulkan/VulkanCommandBuffer.h>
#include <Vulkan/VulkanDescriptorSet.h>
#include <Vulkan/VulkanPipeline.h>
#include <Vulkan/VulkanImage.h>

struct FrameContext
{
	FrameContext(VulkanDevice& device);
	~FrameContext();

	VulkanDevice& device;

	VulkanCommandBufferPool* commandBufferPool;
	VulkanCommandBuffer* commandBuffer;

	VulkanDescriptorPoolSet* frameDescriptorPoolSet;

	VkFence renderFence;
	VkSemaphore swapchainSemaphore, renderSemaphore;
};

class VulkanContext : public GraphicsContext
{
public:
    virtual ~VulkanContext();

	static VulkanContext* get();

    virtual void init(Window* window) override;
	virtual void beginFrame() override;
	virtual void drawFrame() override;
	virtual void endFrame() override;
	QueueFamilyIndices getQueueFamilyIndices() const { return queueFamilyIndices; }
	VmaAllocator getAllocator() const { return allocator; }
	VkDevice getVkDevice() const { return vulkanDevice->getDevice(); }
private:
	void initImmediateCtx();
	void initFrameContext();

	FrameContext* getCurrentFrameContext() { return frameContexts[frameCount % FRAME_OVERLAP]; }
private:
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessager; // Vulkan debug output handle

	VulkanDevice *vulkanDevice;

	std::vector<const char*> deviceExtensions;

	QueueFamilyIndices queueFamilyIndices;

	VulkanSwapChain *swapChain;
	VkSurfaceKHR surface;

	VmaAllocator allocator;

	// draw ctx
	VulkanCommandBufferPool *immediateCmdPool;
	VkFence immediateFence;

	std::vector<FrameContext*> frameContexts;

	VulkanDescriptorPoolSet* globalDescriptorPoolSet;

	VulkanImage *renderImage;

	uint32_t frameCount = 0;
private:
	// blow is for test
	void initComputePipeline();
	void drawComputePipeline(VkCommandBuffer cmd);
	VulkanLayout* computePipelineLayout;
	VulkanComputePipeline* computePipeline;
	VkDescriptorSet computePipelineDescriptorSet;
	struct ComputePipelinePushConstantData 
	{
		glm::vec4 data1;
		glm::vec4 data2;
		glm::vec4 data3;
		glm::vec4 data4;
	} ComputePipelinePushConstantData;
};
#pragma once

#include <Core/Window.h>
#include <RHI/GraphicsContext.h>
#include <Vulkan/VulkanPCH.h>
#include <Vulkan/VulkanSwapChain.h>
#include <Vulkan/VulkanDevice.h>
#include <Vulkan/VulkanCommandBuffer.h>

struct FrameContext
{
	FrameContext(VulkanDevice& device);
	~FrameContext();

	VulkanDevice& device;

	VulkanCommandBufferPool* commandBufferPool;
	VulkanCommandBuffer* commandBuffer;

	VkFence renderFence;
	VkSemaphore swapchainSemaphore, renderSemaphore;
};

class VulkanContext : public GraphicsContext
{
public:
    virtual ~VulkanContext();

    virtual void init(Window* window) override;
	QueueFamilyIndices getQueueFamilyIndices() const { return queueFamilyIndices; }
private:
	void initImmediateCtx();
	void initFrameContext();
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
};
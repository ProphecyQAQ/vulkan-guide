#pragma once

#include <Core/Window.h>
#include <RHI/GraphicsContext.h>
#include <Vulkan/VulkanPCH.h>
#include <Vulkan/VulkanSwapChain.h>
#include <Vulkan/VulkanDevice.h>

class VulkanContext : public GraphicsContext
{
public:
    virtual ~VulkanContext() = default;

    virtual void init(Window* window) override;
	QueueFamilyIndices getQueueFamilyIndices() const { return queueFamilyIndices; }
private:
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessager; // Vulkan debug output handle

	VulkanDevice *vulkanDevice;

	std::vector<const char*> deviceExtensions;

	QueueFamilyIndices queueFamilyIndices;

	VulkanSwapChain *swapChain;
	VkSurfaceKHR surface;

	VmaAllocator allocator;
};
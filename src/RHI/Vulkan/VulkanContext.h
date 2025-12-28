#pragma once

#include <RHI/GraphicsContext.h>
#include <Vulkan/VulkanPCH.h>

class VulkanContext : public GraphicsContext
{
public:
    virtual ~VulkanContext() = default;

    virtual void init(Window* window) override;
private:
	bool isDeviceSuitable(VkPhysicalDevice physicalDevice);
private:
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessager; // Vulkan debug output handle
	VkPhysicalDevice chosenGPU;
	VkDevice device;
	VkQueue graphicsQueue;

	// presentation
	VkSurfaceKHR surface;

	std::vector<const char*> deviceExtensions;
};
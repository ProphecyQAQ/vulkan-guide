#pragma once

#include <Vulkan/VulkanPCH.h>

class VulkanDevice
{
public:
    VulkanDevice(VkInstance instance);

    VkPhysicalDevice getPhysicalDevice() const { return chosenGPU; }
    VkDevice getDevice() const { return device; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
private:
    VkPhysicalDevice selectPhysicalDevice(VkInstance instance);

    bool checkValidationSupport();
    void createDevice();
private:
    VkPhysicalDevice chosenGPU;
	VkDevice device;
	VkQueue graphicsQueue;

	std::vector<const char*> deviceExtensions;
    std::vector<const char*> validationLayer;
};
#pragma once

#include <Vulkan/VulkanPCH.h>

class VulkanDevice
{
public:
    VulkanDevice(VkInstance instance);
    ~VulkanDevice();

    VkPhysicalDevice getPhysicalDevice() const { return chosenGPU; }
    VkDevice getDevice() const { return device; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }

    void setPresnentQueue(VkSurfaceKHR surface);
private:
    VkPhysicalDevice selectPhysicalDevice(VkInstance instance);

    bool checkValidationSupport();
    void createDevice();
private:
    VkPhysicalDevice chosenGPU;
	VkDevice device;
	VkQueue graphicsQueue;
    int32_t graphicsQueueFamilyIndex;
	VkQueue computeQueue;
    int32_t computeQueueFamilyIndex;
	VkQueue transferQueue;
    int32_t transferQueueFamilyIndex;

	std::vector<const char*> deviceExtensions;
    std::vector<const char*> validationLayer;
};
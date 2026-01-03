#pragma once

#include <Vulkan/VulkanPCH.h>
#include <Vulkan/VulkanDevice.h>
#include <SDL_vulkan.h>
#include <Core/Window.h>

class VulkanSwapChain
{
public:
    VulkanSwapChain(VkInstance instance, VulkanDevice* vulkanDevice, VkSurfaceKHR surface, Window *window, uint32_t width, uint32_t height);

    SwapChainSupportDetails getSwapChainSupportDetails();
private:
    void createSwapChain(VkInstance instance, uint32_t width, uint32_t height);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
private:
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    VulkanDevice* vulkanDevice;

    Window *window;

	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
};
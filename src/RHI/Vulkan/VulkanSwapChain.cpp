#include <Core/Log.h>
#include <Vulkan/VulkanSwapChain.h>
#include <Vulkan/VulkanHelper.h>
#include <Vulkan/VulkanContext.h>

VulkanSwapChain::VulkanSwapChain(VkInstance instance, VulkanDevice *vulkanDevice, VkSurfaceKHR surface, Window *window, uint32_t width, uint32_t height)
    : surface(surface), vulkanDevice(vulkanDevice), window(window)
{
    createSwapChain(instance, width, height);

    vulkanDevice->setPresnentQueue(surface);
}

void VulkanSwapChain::createSwapChain(VkInstance instance, uint32_t width, uint32_t height)
{
    SwapChainSupportDetails swapChainSupport = VulkanHeaplerLibrary::querySwapChainSupport(vulkanDevice->getPhysicalDevice(), surface);

    VkSurfaceFormatKHR surfaceFormat = VulkanHeaplerLibrary::selectSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = VulkanHeaplerLibrary::selectSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    LOG_INFO("Select\n surface format: {}\n color space: {}\n presentMode: {}\n extent.width: {} extent.height {}", magic_enum::enum_name(surfaceFormat.format), magic_enum::enum_name(surfaceFormat.colorSpace), magic_enum::enum_name(presentMode), extent.width, extent.height);

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;

    // set image num in swap chain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0)
    {
        imageCount = std::min(imageCount, swapChainSupport.capabilities.maxImageCount);
        imageCount = std::min(imageCount, FRAME_OVERLAP);
    }

    // create swap chain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageExtent = extent;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // only exclusive mode
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(vulkanDevice->getDevice(), &createInfo, nullptr, &swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("[VulkanEngine] [init_swapchain] failed to create swap chain!");
    }
}

SwapChainSupportDetails VulkanSwapChain::getSwapChainSupportDetails()
{
    return VulkanHeaplerLibrary::querySwapChainSupport(vulkanDevice->getPhysicalDevice(), surface);
}

VkExtent2D VulkanSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width{}, height{};
        SDL_Vulkan_GetDrawableSize(reinterpret_cast<SDL_Window*>(window->getNativeWindow()), &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
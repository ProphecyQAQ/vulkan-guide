#include "Core/Log.h"
#include <Vulkan/VulkanHelper.h>

std::vector<VkPhysicalDevice> VulkanHeaplerLibrary::getPhysicalDevices(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    if (deviceCount == 0) {
        throw std::runtime_error("[VulkanHealper] [get_physical_devices] failed to find GPUs with Vulkan support!");
    }

    LOG_INFO("physical device num {}", physicalDevices.size());

    return physicalDevices;
}

std::vector<VkQueueFamilyProperties> VulkanHeaplerLibrary::getQueueFamily(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    LOG_INFO("queue family num {}", queueFamilyProperties.size());

    return queueFamilyProperties;
}

SwapChainSupportDetails VulkanHeaplerLibrary::querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) 
{
    static bool verbose = true;
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    // surface format
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    // present mode
    uint32_t presentCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, nullptr);
    if (presentCount != 0)
    {
        details.presentModes.resize(presentCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, details.presentModes.data());
    }

    if (verbose)
    {
        for (const auto& format:details.formats)
        {
            LOG_INFO("swap chain support format {}, color space {}", magic_enum::enum_name(format.format), magic_enum::enum_name(format.colorSpace));
        }
        for (const auto& presentMode:details.presentModes)
        {
            LOG_INFO("swap chain support present mode {}", magic_enum::enum_name(presentMode));
        }
        verbose = false;
    }

    return details;
}

VkSurfaceFormatKHR VulkanHeaplerLibrary::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
{
    for (const auto& availableFormat : availableFormats) 
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR VulkanHeaplerLibrary::selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes)
{
    for (const auto& availableMode : availableModes)
    {
        if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availableMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

//> create info helper
VkFenceCreateInfo VulkanHeaplerLibrary::fenceCreateInfo(VkFenceCreateFlags flags)
{
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

VkSemaphoreCreateInfo VulkanHeaplerLibrary::semaphoreCreateInfo(VkSemaphoreCreateFlags flags /*= 0*/)
{
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}
//>
#include <vk_healper.h>

std::vector<VkPhysicalDevice> VulkanHeaplerLibrary::get_physical_devices(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    if (deviceCount == 0) {
        throw std::runtime_error("[VulkanHealper] [get_physical_devices] failed to find GPUs with Vulkan support!");
    }

    fmt::println("[VulkanHealper] [get_physical_devices] physical device num {}", physicalDevices.size());

    return physicalDevices;
}

std::vector<VkQueueFamilyProperties> VulkanHeaplerLibrary::get_queue_family(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    fmt::println("[VulkanHealper] [get_queue_family] queue family num {}", queueFamilyProperties.size());

    return queueFamilyProperties;
}

SwapChainSupportDetails VulkanHeaplerLibrary::query_swap_chain_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) 
{
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

    for (const auto& format:details.formats)
    {
        fmt::println("[VulkanHealper] [query_swap_chain_support] swap chain support format {}, color space {}", magic_enum::enum_name(format.format), magic_enum::enum_name(format.colorSpace));
    }

    // present mode
    uint32_t presentCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, nullptr);
    if (presentCount != 0)
    {
        details.presentModes.resize(presentCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, details.presentModes.data());
    }

    for (const auto& presentMode:details.presentModes)
    {
        fmt::println("[VulkanHealper] [query_swap_chain_support] swap chain support present mode {}", magic_enum::enum_name(presentMode));
    }

    return details;
}

VkSurfaceFormatKHR VulkanHeaplerLibrary::select_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
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

VkPresentModeKHR VulkanHeaplerLibrary::select_swap_present_mode(const std::vector<VkPresentModeKHR>& availableModes)
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
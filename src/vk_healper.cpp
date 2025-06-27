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

    fmt::println("[VulkanHealper] [get_queue_family]] queue family num {}", queueFamilyProperties.size());

    return queueFamilyProperties;
}
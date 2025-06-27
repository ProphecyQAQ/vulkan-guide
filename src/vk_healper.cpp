#include <vk_healper.h>

std::vector<VkPhysicalDevice> VulkanHeaplerLibrary::get_physical_devices(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    if (deviceCount == 0) {
        throw std::runtime_error("[VulkanHealper] [GetPhysicalDevices] failed to find GPUs with Vulkan support!");
    }

    fmt::println("[VulkanHealper] [GetPhysicalDevices] physical device num {}", physicalDevices.size());

    return physicalDevices;
}
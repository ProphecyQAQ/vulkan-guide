#include <Core/Window.h>
#include <Vulkan/VulkanContext.h>
#include <vector>
#include <SDL_vulkan.h>
#include <Vulkan/VulkanHelper.h>

void VulkanContext::init(Window* window)
{
    deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // create vkinstance create info
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    // get required extensions from windowing system
    std::vector<const char*> sdlExtestions = window->getVulkanExtensions();

    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(sdlExtestions.size());
    instanceInfo.ppEnabledExtensionNames = sdlExtestions.data();

    // create vkinstance
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }

    // create surface
    if (!SDL_Vulkan_CreateSurface(reinterpret_cast<SDL_Window*>(window->getNativeWindow()), instance, &surface))
    {
        throw std::runtime_error("[VulkanContext] [init] create surface failed");
    }

    std::vector<VkPhysicalDevice> physicalDevices = VulkanHeaplerLibrary::getPhysicalDevices(instance);
    for (const auto& physicalDevice : physicalDevices)
    {
        if (isDeviceSuitable(physicalDevice))
        {
            chosenGPU = physicalDevice;
            break;
        }
    }

    if (chosenGPU != VK_NULL_HANDLE)
    {
        VkPhysicalDeviceProperties chosenGPUProperties;
        vkGetPhysicalDeviceProperties(chosenGPU, &chosenGPUProperties);
        LOG_INFO("chosen GPU {}", chosenGPUProperties.deviceName);
    }
    else 
    {
        throw std::runtime_error("[VulkanEngine] [init_valkan] no chosenGPU");
    }
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    // check extension support
    uint32_t extensionCount; 
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> supportedDeviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, supportedDeviceExtensions.data());

    // check device wether support swapchain
    bool extensionSupport = true;
    for (auto neededDeviceExtension:deviceExtensions)
    {
        if (std::find_if(supportedDeviceExtensions.begin(), supportedDeviceExtensions.end(), [&](VkExtensionProperties& val) 
        {
            return strcmp(val.extensionName, neededDeviceExtension) == 0;
        }) == supportedDeviceExtensions.end())
        {
            extensionSupport = false;
            break;
        }
    }

    bool swapchainAdequate = true;
    SwapChainSupportDetails swapChainSupport = VulkanHeaplerLibrary::querySwapChainSupport(physicalDevice, surface);
    if (extensionSupport)
    {
        swapchainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader && extensionSupport && swapchainAdequate;
}
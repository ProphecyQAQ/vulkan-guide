#include <Core/Log.h>
#include <Vulkan/VulkanContext.h>
#include <vector>
#include <SDL_vulkan.h>
#include <Vulkan/VulkanHelper.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

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
        throw std::runtime_error("[VulkanSwapChain] [init] create surface failed");
    }

    // create vulkan device
    vulkanDevice = new VulkanDevice(instance);

    // Init swapchain
    swapChain = new VulkanSwapChain(instance, vulkanDevice, surface, window, 800, 600);

    // init vma allocator
    VmaAllocatorCreateInfo vmaCreateInfo{};
    vmaCreateInfo.physicalDevice = vulkanDevice->getPhysicalDevice();
    vmaCreateInfo.device = vulkanDevice->getDevice();
    vmaCreateInfo.instance = instance;
    vmaCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&vmaCreateInfo, &allocator);
}
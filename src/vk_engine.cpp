//> includes
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>

#include <chrono>
#include <thread>
#include <cstring>

VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }
void VulkanEngine::init()
{
    // only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        _windowExtent.width,
        _windowExtent.height,
        window_flags);

    _validationLayer.push_back("VK_LAYER_KHRONOS_validation");
    _deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();

    // everything went fine
    _isInitialized = true;
}

bool VulkanEngine::check_validation_support()
{
    // get supported validation layer properties
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


    // check support
    for (const char *layerName : _validationLayer)
    {
        bool isFound = false;
        for (const auto &layerProperties : availableLayers)
        {
            if ( strcmp(layerProperties.layerName, layerName) == 0 )
            {
                isFound = true;
                break;
            }
        }

        if (!isFound) 
        {
            return false;
        }
    }

    return true;
}

bool VulkanEngine::is_device_suitable(VkPhysicalDevice physicalDevice)
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
    for (auto neededDeviceExtension:_deviceExtensions)
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
    SwapChainSupportDetails swapChainSupport = VulkanHeaplerLibrary::query_swap_chain_support(physicalDevice, _surface);
    if (extensionSupport)
    {
        swapchainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader && extensionSupport && swapchainAdequate;
}

bool VulkanEngine::is_queue_family_suitable_for_graphics(VkQueueFamilyProperties queueFamilyProperty)
{
    if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
        return true;
    }
    return false;
}

bool VulkanEngine::is_queue_family_suitable_for_presentation(VkQueueFamilyProperties queueFamilyProperty, uint32_t queueFamilyIndex)
{
    VkBool32 presentationSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(_chosenGPU, queueFamilyIndex, _surface, &presentationSupport);
    if (presentationSupport)
    {
        return true;
    }
    return false;
}

void VulkanEngine::init_vulkan()
{
    // create application info, not necessary
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // create vkinstance create info
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    // create validation info
    if (check_validation_support()) 
    {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayer.size());
        instanceInfo.ppEnabledLayerNames = _validationLayer.data();
    }
    else 
    {
        instanceInfo.enabledLayerCount = 0;
    }

    // get extension to interface with window
    unsigned int sdlExtensionCount = 0;
    std::vector<const char*> sdlExtensions;

    if (SDL_Vulkan_GetInstanceExtensions(_window, &sdlExtensionCount, nullptr) == false)
    {
        fmt::println("[VulkanEngine] [init_vulkan] get instance extestions failed");
        return;
    }

    sdlExtensions.resize(sdlExtensionCount);
    if (SDL_Vulkan_GetInstanceExtensions(_window, &sdlExtensionCount, sdlExtensions.data()) == false)
    {
        fmt::println("[VulkanEngine] [init_vulkan] get instance extestions failed");
        return;
    }

    for (unsigned int i = 0; i < sdlExtensionCount; i ++)
    {
        fmt::println("[VulkanEngine] [init_valkan] SDL Extension: {}", sdlExtensions[i]);
    }
    instanceInfo.enabledExtensionCount = sdlExtensionCount;
    instanceInfo.ppEnabledExtensionNames = sdlExtensions.data();

    // create vkinstance
    if (vkCreateInstance(&instanceInfo, nullptr, &_instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }

    // create surface
    if (!SDL_Vulkan_CreateSurface(_window, _instance, &_surface))
    {
        throw std::runtime_error("[VulkanEngine] [init_swapchain] create surface failed");
    }


    // get physical device
    std::vector<VkPhysicalDevice> physicalDevices = VulkanHeaplerLibrary::get_physical_devices(_instance);
    for (const auto& physicalDevice : physicalDevices)
    {
        if (is_device_suitable(physicalDevice))
        {
            _chosenGPU = physicalDevice;
            break;
        }
    }

    if (_chosenGPU != VK_NULL_HANDLE)
    {
        VkPhysicalDeviceProperties chosenGPUProperties;
        vkGetPhysicalDeviceProperties(_chosenGPU, &chosenGPUProperties);
        fmt::println("[VulkanEngine] [init_valkan] chosen GPU {}", chosenGPUProperties.deviceName);
    }
    else 
    {
        throw std::runtime_error("[VulkanEngine] [init_valkan] no chosenGPU");
    }

    // create queue family
    std::vector<VkQueueFamilyProperties> queueFamilyProperties = VulkanHeaplerLibrary::get_queue_family(_chosenGPU);
    QueueFamilyIndices indices;
    for (int i = 0; i < queueFamilyProperties.size(); i ++)
    {
        if (is_queue_family_suitable_for_graphics(queueFamilyProperties[i]))
        {
            indices.graphicsFamily = i;
        }
        if (is_queue_family_suitable_for_presentation(queueFamilyProperties[i], i))
        {
            indices.presentFamily = i;
        }

        if (indices.is_complete())
        {
            break;
        }
    }
    if (indices.is_complete())
    {
        fmt::println("[VulkanEngine] [init_valkan] find graphics queue family {}, presentation family {}", indices.graphicsFamily.value(), indices.presentFamily.value());
    }
    else 
    {
        throw std::runtime_error("[VulkanEngine] [init_valkan] no graphics queue family");
    }

    // set up a logical device

    // create queue info
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    for (auto queueFamilyIndex:uniqueQueueFamilies) 
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Specifying used device features
    VkPhysicalDeviceFeatures deviceFeatures{};

    // create logical device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = _deviceExtensions.data();

    if (check_validation_support()) 
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayer.size());
        deviceCreateInfo.ppEnabledLayerNames = _validationLayer.data();
    }
    else 
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(_chosenGPU, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS)
    {
        throw std::runtime_error("[VulkanEngine] [init_valkan] failed to create logical device!");
    }

    // Get queue handle
    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
}

void VulkanEngine::init_swapchain()
{
    // need check:
    // Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
    // Surface formats (pixel format, color space)
    // Available presentation modes

}

void VulkanEngine::init_commands()
{
    //nothing yet
}
void VulkanEngine::init_sync_structures()
{
    //nothing yet
}

void VulkanEngine::cleanup()
{
    if (_isInitialized) {

        SDL_DestroyWindow(_window);
    }

    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);

    // clear engine pointer
    loadedEngine = nullptr;
}

void VulkanEngine::draw()
{
    // nothing yet
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    stop_rendering = false;
                }
            }
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}
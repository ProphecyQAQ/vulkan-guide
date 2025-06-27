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

    validationLayer.push_back("VK_LAYER_KHRONOS_validation");

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
    for (const char *layerName : validationLayer)
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
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayer.size());
        instanceInfo.ppEnabledLayerNames = validationLayer.data();
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
        fmt::println("SDL Extension: {}", sdlExtensions[i]);
    }

    // create vkinstance
    if (vkCreateInstance(&instanceInfo, nullptr, &_instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }


    // get physical device
    std::vector<VkPhysicalDevice> physicalDevices = VulkanHeaplerLibrary::GetPhysicalDevices(_instance);
}
void VulkanEngine::init_swapchain()
{
    //nothing yet
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
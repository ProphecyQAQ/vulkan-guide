//> includes
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_images.h>
#include <vk_types.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

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

    init_descriptors();

    init_pipelines();

    init_imgui();

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

QueueFamilyIndices VulkanEngine::find_queue_families(VkPhysicalDevice device)
{
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

    return indices;
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

VkExtent2D VulkanEngine::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width{}, height{};
        SDL_Vulkan_GetDrawableSize(_window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
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
    QueueFamilyIndices indices = find_queue_families(_chosenGPU);

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
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceFeatures{};
    bufferDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceFeatures.bufferDeviceAddress = true;
    VkPhysicalDeviceSynchronization2Features sync2Features{};
    sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Features.synchronization2 = true;
    sync2Features.pNext = &bufferDeviceFeatures;

    // create logical device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = _deviceExtensions.data();
    deviceCreateInfo.pNext = &sync2Features;

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

    // init vma lib
    VmaAllocatorCreateInfo vmaCreateInfo{};
    vmaCreateInfo.physicalDevice = _chosenGPU;
    vmaCreateInfo.device = _device;
    vmaCreateInfo.instance = _instance;
    vmaCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&vmaCreateInfo, &_allocator);
    _mainDeletionQueue.push_function([&]() {
        vmaDestroyAllocator(_allocator);
    });
}

void VulkanEngine::init_swapchain()
{
    // three types of settings to determine:

    // Surface format (color depth)
    // Presentation mode (conditions for "swapping" images to the screen)
    // Swap extent (resolution of images in swap chain)

    SwapChainSupportDetails swapChainSupport = VulkanHeaplerLibrary::query_swap_chain_support(_chosenGPU, _surface);

    VkSurfaceFormatKHR surfaceFormat = VulkanHeaplerLibrary::select_swap_surface_format(swapChainSupport.formats);
    VkPresentModeKHR presentMode = VulkanHeaplerLibrary::select_swap_present_mode(swapChainSupport.presentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities);

    fmt::println("[VulkanEngine] [init_swapchain] select\n surface format: {}\n color space: {}\n presentMode: {}\n extent.width: {} extent.height {}", magic_enum::enum_name(surfaceFormat.format), magic_enum::enum_name(surfaceFormat.colorSpace), magic_enum::enum_name(presentMode), extent.width, extent.height);
    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;

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
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageExtent = extent;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices = find_queue_families(_chosenGPU);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (indices.graphicsFamily.value() == indices.presentFamily.value())
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }   
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        createInfo.queueFamilyIndexCount = 2;
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("[VulkanEngine] [init_swapchain] failed to create swap chain!");
    }

    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(_device, _swapChain, &swapChainImageCount, nullptr);
    _swapChainImage.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(_device, _swapChain, &swapChainImageCount, _swapChainImage.data());

    // create image view for swap chain image
    _swapChainImageView.resize(swapChainImageCount);
    for (uint32_t i = 0; i < swapChainImageCount; i ++)
    {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = _swapChainImage[i];
        viewCreateInfo.format = _swapChainImageFormat;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(_device, &viewCreateInfo, nullptr, &_swapChainImageView[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("[VulkanEngine] [init_swapchain] failed to create image views!");
        }
    }

    // create draw resource
    VkExtent3D drawImageExtent =  {
        _windowExtent.width,
        _windowExtent.height,
        1
    };

    // set image format
    _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsage;
    drawImageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_STORAGE_BIT |
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo imageCreateInfo = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsage, drawImageExtent);

    // need allocat it from gpu local memoty
    VmaAllocationCreateInfo imageAllocationInfo{};
    imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create image
    vmaCreateImage(_allocator, &imageCreateInfo, &imageAllocationInfo, &_drawImage.image, &_drawImage.allocation, nullptr);

    // build image view
    VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_device, &imageViewInfo, nullptr, &_drawImage.imageView));

    // add to deletion queue
    _mainDeletionQueue.push_function([=]() {
        vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
        vkDestroyImageView(_device, _drawImage.imageView, nullptr);
    });
}   

void VulkanEngine::init_commands()
{
    // Alloc vkCommanderBuffer from VkCommanderPool
    // using vkCmdxxx records cmd into commander buffer
    // submit commander buffrt into vkQueue

    QueueFamilyIndices indices = find_queue_families(_chosenGPU);

    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(indices.graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i ++)
    {
        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frameData[i]._commandPool));
        
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frameData[i]._commandPool);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frameData[i]._commandBuffer));
    }

    // creat cmd object for immediate submits
    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool));

    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immCommandBuffer));
    _mainDeletionQueue.push_function([=]()
    {
        vkDestroyCommandPool(_device, _immCommandPool, nullptr);
    });
}

void VulkanEngine::init_sync_structures()
{
    // create syncronization structures
    // fence to control when gpu has finished rendering current frame
    // 2 semaphores to syncronize rendering with swapchain

    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i ++)
    {
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frameData[i]._renderFence));

        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frameData[i]._renderSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frameData[i]._swapchainSemaphore));
    }

    // create syncronization structures for immediate submit
    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));
    _mainDeletionQueue.push_function([=]()
    {
        vkDestroyFence(_device, _immFence, nullptr);
    });
}

void VulkanEngine::init_descriptors()
{
    // create a descriptor pool that will hold 10 sets with 1 image
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
    };

    globalDescriptorAllocator.init_pool(_device, 10, sizes);

    // make descriptor layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    // allocate a descriptor set for our draw image
    _drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = _drawImage.imageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    
    // Structure specifying the parameters of a descriptor set write operation
    VkWriteDescriptorSet drawImageWrite{};
    drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    drawImageWrite.pNext = nullptr;
    drawImageWrite.pImageInfo = &imageInfo;
    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = _drawImageDescriptors;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

    _mainDeletionQueue.push_function([&]()
    {
        globalDescriptorAllocator.destroy_pool(_device);
        vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
    }
    );
}

void VulkanEngine::init_pipelines()
{
    init_background_pipelines();
}

void VulkanEngine::init_background_pipelines()
{   
    // create pipeline layout
    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
    computeLayout.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

    // load shader module
    VkShaderModule computeDrawShader;
    if (vkutil::load_shader_module("../../shaders/gradient.comp.spv", _device, &computeDrawShader) == false)
    {
        fmt::print("[VulkanEngine] [init_background_pipelines] Error when building the compute shader \n");
    }

    VkPipelineShaderStageCreateInfo computeShaderStage{};
    computeShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStage.pNext = nullptr;
    computeShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStage.module = computeDrawShader;
    computeShaderStage.pName = "main";

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.pNext = nullptr;
    computePipelineInfo.layout = _gradientPipelineLayout;
    computePipelineInfo.stage = computeShaderStage;

    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &_gradientPipeline));

    // clean up
    vkDestroyShaderModule(_device, computeDrawShader, nullptr);

    _mainDeletionQueue.push_function([&]()
    {
        vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _gradientPipeline, nullptr);
    });
}

void VulkanEngine::init_imgui()
{
    // 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _chosenGPU;
	init_info.Device = _device;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;

	//dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapChainImageFormat;
	

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();

	// add the destroy the imgui created structures
	_mainDeletionQueue.push_function([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
	});
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_CHECK(vkResetFences(_device, 1, &_immFence));
	VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

    VkCommandBuffer cmd = _immCommandBuffer;

    VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    // submit
    VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submitInfo = vkinit::submit_info(&cmdSubmitInfo, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo, _immFence));
    VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, UINT64_MAX));
}

void VulkanEngine::cleanup()
{
    if (_isInitialized) {

        vkDeviceWaitIdle(_device);

        for (auto &view : _swapChainImageView)
        {
            vkDestroyImageView(_device, view, nullptr);
        }

        for (int i = 0; i < FRAME_OVERLAP; i ++)
        {
            vkDestroyCommandPool(_device, _frameData[i]._commandPool, nullptr);

            // destroy all sync struct
            vkDestroyFence(_device, _frameData[i]._renderFence, nullptr);
            vkDestroySemaphore(_device, _frameData[i]._swapchainSemaphore, nullptr);
            vkDestroySemaphore(_device, _frameData[i]._renderSemaphore, nullptr);

            _frameData[i]._deletionQueue.flush();
        }

        _mainDeletionQueue.flush();

        vkDestroySwapchainKHR(_device, _swapChain, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyDevice(_device, nullptr);
        vkDestroyInstance(_instance, nullptr);

        SDL_DestroyWindow(_window);
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

void VulkanEngine::draw_background(VkCommandBuffer cmd)
{
    // make a clear-color
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(_frameNumber / 120.f));
    clearValue = { {0.f, 0.f, flash, 1.f} };

    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    // clear image
    vkCmdClearColorImage(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    // bind gradient compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);

    // bind descriptor set containing the draw image for the compute pipeline
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

    // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
    vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
}

void VulkanEngine::draw()
{ 
    FrameData& currentFrame = get_current_frame();

    // wait until the gpu has finished rendering the last frame
    VK_CHECK(vkWaitForFences(_device, 1, &currentFrame._renderFence, true, UINT64_MAX));
    currentFrame._deletionQueue.flush();
    VK_CHECK(vkResetFences(_device, 1, &currentFrame._renderFence));

    _drawExtent.width = _drawImage.imageExtent.width;
    _drawExtent.height = _drawImage.imageExtent.height;

    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapChain, UINT64_MAX, currentFrame._swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd = currentFrame._commandBuffer;
    
    // reset cmd before using
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // create cmd begin info, and will use it once
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // transition the draw image into writeable image
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // draw image
    draw_background(cmd);

    // transition the draw image and swapchain image into transfer layout
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, _swapChainImage[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // copy draw image to swapchain image
    vkutil::copy_image_to_image(cmd, _drawImage.image, _swapChainImage[swapchainImageIndex], _drawExtent, _swapChainExtent);

    {
        // draw imgui

        // transition swapchain image to color attachment layout
        vkutil::transition_image(cmd, _swapChainImage[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        draw_imgui(cmd, _swapChainImageView[swapchainImageIndex]);
    }

    // transfer swapchian image to present layout
    vkutil::transition_image(cmd, _swapChainImage[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // finish command buffer
    VK_CHECK(vkEndCommandBuffer(cmd));

    // prepare for submit to the queue
    // wait on the _presentSemaphore, as that semaphore is signaled when the swapchian is ready
    // will signal the _renderSemaphore, to signal that rendering has finished
    VkCommandBufferSubmitInfo cmdSubmitInfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitSemaInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrame._swapchainSemaphore);
    VkSemaphoreSubmitInfo signalSeamInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrame._renderSemaphore);

    VkSubmitInfo2 submitInfo = vkinit::submit_info(&cmdSubmitInfo, &signalSeamInfo, &waitSemaInfo);

    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo, currentFrame._renderFence));

    // prepare for present
    // put image we just rendered into window
    // need wait on _renderSemaphore
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pSwapchains = &_swapChain;
    presentInfo.swapchainCount = 1;
    presentInfo.pNext = nullptr;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &currentFrame._renderSemaphore;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    _frameNumber ++;
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

            //send SDL event to imgui for handling
            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // imgui new frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        //some imgui UI to test
        ImGui::ShowDemoWindow();

        //make imgui calculate internal draw structures
        ImGui::Render();

        draw();
    }
}

void VulkanEngine::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
    VkRenderingAttachmentInfo attachmentInfo = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = vkinit::rendering_info(_swapChainExtent, &attachmentInfo, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}
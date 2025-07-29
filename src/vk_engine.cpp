//> includes
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <glm/gtx/transform.hpp>

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

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        _windowExtent.width,
        _windowExtent.height,
        window_flags);

    _validationLayer.push_back("VK_LAYER_KHRONOS_validation");
    _deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    resize_requested = false;

    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();

    init_descriptors();

    init_pipelines();

    init_imgui();

    init_default_data();

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
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeatures.dynamicRendering = true;
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceFeatures{};
    bufferDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceFeatures.bufferDeviceAddress = true;
    bufferDeviceFeatures.pNext = &dynamicRenderingFeatures;
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

    create_swapchain();
    
    // create draw resource
    VkExtent3D drawImageExtent =  {
        _windowExtent.width,
        _windowExtent.height,
        1
    };

    // create draw image
    {
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

    // create depth image
    {   
        _depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
        _depthImage.imageExtent = drawImageExtent;
        VkImageUsageFlags usages{};
        usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkImageCreateInfo imageCreateInfo = vkinit::image_create_info(_depthImage.imageFormat, usages, drawImageExtent);

        // need allocat it from gpu local memoty
        VmaAllocationCreateInfo imageAllocationInfo{};
        imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // allocate and create image
        vmaCreateImage(_allocator, &imageCreateInfo, &imageAllocationInfo, &_depthImage.image, &_depthImage.allocation, nullptr);

        // build image view
        VkImageViewCreateInfo viewCreateInfo = vkinit::imageview_create_info(_depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
        VK_CHECK(vkCreateImageView(_device, &viewCreateInfo, nullptr, &_depthImage.imageView));

        _mainDeletionQueue.push_function([=]()
        {
            vkDestroyImageView(_device, _depthImage.imageView, nullptr);
            vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);
        });
    }
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
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };

    globalDescriptorAllocator.init(_device, 10, sizes);
    
    // make descriptor layout for gpu scene data
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _gpuSceneDataDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    // make descriptor layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    // make descriptor layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _singleImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    // allocate a descriptor set for our draw image
    _drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

    // write the descriptor set
    DescriptorWriter writer;
    writer.write_image(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.update_set(_device, _drawImageDescriptors);

    // init frame data desriptor
    for (int i = 0; i < FRAME_OVERLAP; i ++)
    {
        // create a descriptor pool for each frame
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_size = {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
        };

        _frameData[i]._frameDescriptors = DescriptorAllocatorGrowable{};
        _frameData[i]._frameDescriptors.init(_device, 1000, frame_size);

        _mainDeletionQueue.push_function([&, i](){
            _frameData[i]._frameDescriptors.destroy_pools(_device);
        });
    }

    _mainDeletionQueue.push_function([&]()
    {
        globalDescriptorAllocator.destroy_pools(_device);
        vkDestroyDescriptorSetLayout(_device, _singleImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _gpuSceneDataDescriptorLayout, nullptr);
    }
    );
}

void VulkanEngine::init_pipelines()
{
    init_background_pipelines();

    // graphics
    init_mesh_pipeline();

    // material
    _metalRoughMaterial.build_pipelines(this);
}

void VulkanEngine::init_background_pipelines()
{   
    // create constant
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.size = sizeof(ComputePushConstants);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;

    // create pipeline layout
    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
    computeLayout.setLayoutCount = 1;
    computeLayout.pPushConstantRanges = &pushConstantRange;
    computeLayout.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

    VkShaderModule gradientShader;
    if (!vkutil::load_shader_module("../../shaders/gradient_color.comp.spv", _device, &gradientShader)) {
        fmt::print("Error when building the compute shader \n");
    }

    VkShaderModule skyShader;
    if (!vkutil::load_shader_module("../../shaders/sky.comp.spv", _device, &skyShader)) {
        fmt::print("Error when building the compute shader \n");
    }

    VkPipelineShaderStageCreateInfo stageinfo{};
    stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageinfo.pNext = nullptr;
    stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageinfo.module = gradientShader;
    stageinfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.layout = _gradientPipelineLayout;
    computePipelineCreateInfo.stage = stageinfo;

    ComputeEffect gradient;
    gradient.layout = _gradientPipelineLayout;
    gradient.name = "gradient";
    gradient.data = {};

    //default colors
    gradient.data.data1 = glm::vec4(1, 0, 0, 1);
    gradient.data.data2 = glm::vec4(0, 0, 1, 1);

    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));

    //change the shader module only to create the sky shader
    computePipelineCreateInfo.stage.module = skyShader;

    ComputeEffect sky;
    sky.layout = _gradientPipelineLayout;
    sky.name = "sky";
    sky.data = {};
    //default sky parameters
    sky.data.data1 = glm::vec4(0.1, 0.2, 0.4 ,0.97);

    VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

    //add the 2 background effects into the array
    backgroundEffects.push_back(gradient);
    backgroundEffects.push_back(sky);

    //destroy structures properly
    vkDestroyShaderModule(_device, gradientShader, nullptr);
    vkDestroyShaderModule(_device, skyShader, nullptr);
    _mainDeletionQueue.push_function([=]() {
        vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
        vkDestroyPipeline(_device, sky.pipeline, nullptr);
        vkDestroyPipeline(_device, gradient.pipeline, nullptr);
    });
}

void VulkanEngine::init_mesh_pipeline()
{
    VkShaderModule triangleFragShader;
	if (!vkutil::load_shader_module("../../shaders/tex_image.frag.spv", _device, &triangleFragShader)) {
		fmt::print("Error when building the triangle fragment shader module");
	}
	else {
		fmt::print("Triangle fragment shader succesfully loaded");
	}

	VkShaderModule triangleVertexShader;
	if (!vkutil::load_shader_module("../../shaders/colored_triangle_mesh.vert.spv", _device, &triangleVertexShader)) {
		fmt::print("Error when building the triangle vertex shader module");
	}
	else {
		fmt::print("Triangle vertex shader succesfully loaded");
	}

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(GPUDrawPushConstant);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo layoutCreateInfo = vkinit::pipeline_layout_create_info();
    layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    layoutCreateInfo.pushConstantRangeCount = 1;
    layoutCreateInfo.pSetLayouts = &_singleImageDescriptorLayout;
    layoutCreateInfo.setLayoutCount = 1;
    
    VK_CHECK(vkCreatePipelineLayout(_device, &layoutCreateInfo, nullptr, &_meshPipelineLayout));

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.set_pipeline_layout(_meshPipelineLayout);
    //connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.set_shader(triangleVertexShader, triangleFragShader);
	//it will draw triangles
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.set_multisampling_none();
	//no blending
	pipelineBuilder.disable_blending();
    //pipelineBuilder.enable_blend_additive();
	//pipelineBuilder.disable_depthtest();
    pipelineBuilder.enable_depthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);

    //connect the image format we will draw into, from draw image
	pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
	pipelineBuilder.set_depth_format(_depthImage.imageFormat);

	//finally build the pipeline
	_meshPipeline = pipelineBuilder.build_pipeline(_device);

    vkDestroyShaderModule(_device, triangleFragShader, nullptr);
    vkDestroyShaderModule(_device, triangleVertexShader, nullptr);
    _mainDeletionQueue.push_function([&]()
    {
        vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _meshPipeline, nullptr);
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
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchainImageFormat;
	

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();

	// add the destroy the imgui created structures
	_mainDeletionQueue.push_function([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
	});
}

void VulkanEngine::init_default_data()
{
    // load a basic mesh from gltf file
    testMeshes = loadGltfMeshes(this,"..\\..\\assets\\basicmesh.glb").value();

    // init default image data
    // 3 default textures, white, grey, black. 1 pixel each
    uint32_t white = glm::packUnorm4x8(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    _whiteImage = create_image((void*)&white, VkExtent3D{1,1,1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    _blackImage = create_image((void*)&black, VkExtent3D{1,1,1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1.0f));
    _greyImage = create_image((void*)&grey, VkExtent3D{1,1,1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    // checkboard image
    std::array<uint32_t, 16*16> pixels;
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    for (int i = 0; i < 16; i ++)
    {
        for (int j = 0; j < 16; j ++)
        {
            pixels[i * 16 + j] = ((i % 2) ^ (j % 2)) ? magenta : black;
        }
    }
    _errorCheckerboardImage = create_image((void*)pixels.data(), VkExtent3D{16,16,1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    // create default sampler
    VkSamplerCreateInfo samplerCreateInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;

    VK_CHECK(vkCreateSampler(_device, &samplerCreateInfo, nullptr, &_defaultSamplerLinear));

    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;

    VK_CHECK(vkCreateSampler(_device, &samplerCreateInfo, nullptr, &_defaultSamplerNearest));

    _mainDeletionQueue.push_function([&](){
        destroy_image(_whiteImage);
        destroy_image(_blackImage);
        destroy_image(_greyImage);
        destroy_image(_errorCheckerboardImage);

        vkDestroySampler(_device, _defaultSamplerLinear, nullptr);
        vkDestroySampler(_device, _defaultSamplerNearest, nullptr);
    });

    // create material data
    {
        GLTFMetallic_Roughness::MaterialResources materialResources;
        materialResources.colorImage = _whiteImage;
        materialResources.colorSampler = _defaultSamplerLinear;
        materialResources.metalRoughImage = _whiteImage;
        materialResources.metalRoughSampler = _defaultSamplerLinear;

        AllocatedBuffer materialConstantsBuffer = create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        // write the buffer
        GLTFMetallic_Roughness::MaterialConstants* sceneUniformData = (GLTFMetallic_Roughness::MaterialConstants*)materialConstantsBuffer.allocation->GetMappedData();
        sceneUniformData->colorFactors = glm::vec4{1,1,1,1};
        sceneUniformData->metal_rough_factors = glm::vec4{1,0.5,0,0};

        _mainDeletionQueue.push_function([=, this](){
            destroy_buffer(materialConstantsBuffer);
        });

        materialResources.dataBuffer = materialConstantsBuffer.buffer;
        materialResources.dataBufferOffset = 0;

        _defaultMaterialInstance = _metalRoughMaterial.write_material(_device, MaterialPass::MainColor, materialResources, globalDescriptorAllocator);


        for (auto& mesh : testMeshes)
        {
            std::shared_ptr<MeshNode> meshNode = std::make_shared<MeshNode>();
            meshNode->mesh = mesh;
            meshNode->localTransform = glm::mat4{1.f};
            meshNode->worldTransform = glm::mat4{1.f};

            for (auto& surface : meshNode->mesh->surfaces)
            {
                surface.material = std::make_shared<GLTFMaterial>(_defaultMaterialInstance);
            }

            loadedNodes[mesh->name] = std::move(meshNode);
            fmt::println("[VulkanEngine] [init_default_data] loaded mesh {}", mesh->name);
        }
    }
}

void VulkanEngine::create_swapchain()
{
    static bool verbose = true;
    SwapChainSupportDetails swapChainSupport = VulkanHeaplerLibrary::query_swap_chain_support(_chosenGPU, _surface);

    VkSurfaceFormatKHR surfaceFormat = VulkanHeaplerLibrary::select_swap_surface_format(swapChainSupport.formats);
    VkPresentModeKHR presentMode = VulkanHeaplerLibrary::select_swap_present_mode(swapChainSupport.presentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities);

    if (verbose)
    {
        fmt::println("[VulkanEngine] [init_swapchain] select\n surface format: {}\n color space: {}\n presentMode: {}\n extent.width: {} extent.height {}", magic_enum::enum_name(surfaceFormat.format), magic_enum::enum_name(surfaceFormat.colorSpace), magic_enum::enum_name(presentMode), extent.width, extent.height);
        verbose = false;
    }
    _swapchainImageFormat = surfaceFormat.format;
    _swapchainExtent = extent;

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

    if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("[VulkanEngine] [init_swapchain] failed to create swap chain!");
    }

    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(_device, _swapchain, &swapChainImageCount, nullptr);
    _swapchainImage.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(_device, _swapchain, &swapChainImageCount, _swapchainImage.data());

    // create image view for swap chain image
    _swapchainImageView.resize(swapChainImageCount);
    for (uint32_t i = 0; i < swapChainImageCount; i ++)
    {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = _swapchainImage[i];
        viewCreateInfo.format = _swapchainImageFormat;
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
        
        if (vkCreateImageView(_device, &viewCreateInfo, nullptr, &_swapchainImageView[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("[VulkanEngine] [init_swapchain] failed to create image views!");
        }
    }
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

void VulkanEngine::destroy_swapchain()
{
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    for (auto &view : _swapchainImageView)
    {
        vkDestroyImageView(_device, view, nullptr);
    }
}

void VulkanEngine::cleanup()
{
    if (_isInitialized) {

        vkDeviceWaitIdle(_device);

        for (int i = 0; i < FRAME_OVERLAP; i ++)
        {
            vkDestroyCommandPool(_device, _frameData[i]._commandPool, nullptr);

            // destroy all sync struct
            vkDestroyFence(_device, _frameData[i]._renderFence, nullptr);
            vkDestroySemaphore(_device, _frameData[i]._swapchainSemaphore, nullptr);
            vkDestroySemaphore(_device, _frameData[i]._renderSemaphore, nullptr);

            _frameData[i]._deletionQueue.flush();
        }

        // clean up mesh
        for (auto& mesh : testMeshes) {
            destroy_buffer(mesh->meshBuffers.indexBuffer);
            destroy_buffer(mesh->meshBuffers.vertexBuffer);
        }

        _mainDeletionQueue.flush();

        destroy_swapchain();
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

	ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];

	// bind the background compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

	vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
}

void VulkanEngine::draw_geometry(VkCommandBuffer cmd)
{
    // connected to draw image
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);

    // set dynamic viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_drawExtent.width);
    viewport.height = static_cast<float>(_drawExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = _drawExtent.width;
    scissor.extent.height = _drawExtent.height;

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // create gpu scene data
    
    // allocate uniform buffer for scene data
    AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    // add destroy
    get_current_frame()._deletionQueue.push_function([=]() {
        destroy_buffer(gpuSceneDataBuffer);
    });

    // write the buffer
    GPUSceneData *sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
    *sceneUniformData = _sceneData;

    // create descriptor set that binds that buffer and update it
    VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout, nullptr);

    DescriptorWriter writer;
    writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(_device, globalDescriptor);
    
    // draw context
    for (const RenderObject& obj : _mainDrawContext.opaqueSurface)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.material->pipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.material->pipeline->layout, 1, 1, &obj.material->materialSet, 0, nullptr);

        vkCmdBindIndexBuffer(cmd, obj.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        GPUDrawPushConstant pushConstant;
        pushConstant.vertexBuffer = obj.vertexBufferAddress;
        pushConstant.worldMatrix = obj.transform;
        vkCmdPushConstants(cmd, obj.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstant), &pushConstant);

        vkCmdDrawIndexed(cmd, obj.indexCount, 1, obj.firstIndex, 0, 0);
    }

    vkCmdEndRendering(cmd);
}

void VulkanEngine::update_scene(float deltaTime)
{
    _mainDrawContext.opaqueSurface.clear();

    loadedNodes["Suzanne"]->Draw(glm::mat4{1.f}, _mainDrawContext);	

    for (int x = -3; x < 3; x++) {

		glm::mat4 scale = glm::scale(glm::vec3{0.2});
		glm::mat4 translation =  glm::translate(glm::vec3{x, 1, 0});

		loadedNodes["Cube"]->Draw(translation * scale, _mainDrawContext);
	}

	_sceneData.view = glm::translate(glm::vec3{ 0,0,-5 });
	// camera projection
	_sceneData.proj = glm::perspective(glm::radians(70.f), (float)_windowExtent.width / (float)_windowExtent.height, 10000.f, 0.1f);

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	_sceneData.proj[1][1] *= -1;
	_sceneData.viewProj = _sceneData.proj * _sceneData.view;

	//some default lighting parameters
	_sceneData.ambientColor = glm::vec4(.1f);
	_sceneData.sunlightColor = glm::vec4(1.f);
	_sceneData.sunlightDirection = glm::vec4(0,1,0.5,1.f);
}

void VulkanEngine::draw()
{ 
    FrameData& currentFrame = get_current_frame();
    
    // update scene data and draw context
    update_scene();

    // wait until the gpu has finished rendering the last frame
    VK_CHECK(vkWaitForFences(_device, 1, &currentFrame._renderFence, true, UINT64_MAX));
    currentFrame._deletionQueue.flush();
    currentFrame._frameDescriptors.clear_pools(_device);
    VK_CHECK(vkResetFences(_device, 1, &currentFrame._renderFence));

    _drawExtent.width = _drawImage.imageExtent.width;
    _drawExtent.height = _drawImage.imageExtent.height;

    uint32_t swapchainImageIndex;
	VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (e == VK_ERROR_OUT_OF_DATE_KHR) {
        resize_requested = true;       
		return ;
	}

    VkCommandBuffer cmd = currentFrame._commandBuffer;
    
    // reset cmd before using
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // create cmd begin info, and will use it once
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // draw background
    {
        // transition the draw image into writeable image
        vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        // draw image
        draw_background(cmd);
    }

    // draw gemotroy
    {
        // transition the draw image into color attachment layout
        vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // draw geometry
        draw_geometry(cmd);
    }

    // transition the draw image and swapchain image into transfer layout
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, _swapchainImage[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // copy draw image to swapchain image
    vkutil::copy_image_to_image(cmd, _drawImage.image, _swapchainImage[swapchainImageIndex], _drawExtent, _swapchainExtent);

    {
        // draw imgui

        // transition swapchain image to color attachment layout
        vkutil::transition_image(cmd, _swapchainImage[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        draw_imgui(cmd, _swapchainImageView[swapchainImageIndex]);
    }

    // transfer swapchian image to present layout
    vkutil::transition_image(cmd, _swapchainImage[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pNext = nullptr;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &currentFrame._renderSemaphore;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VkResult presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        resize_requested = true;
        return;
    }

    _frameNumber ++;
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;
    static auto last_frame_time = std::chrono::high_resolution_clock::now();

    // main loop
    while (!bQuit) {
        auto current_frame_time = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(current_frame_time - last_frame_time).count();
        last_frame_time = current_frame_time;

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
        if (resize_requested) {
            resize_swapchain();
        }

        // imgui new frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("background")) {
			
			ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];
		
			ImGui::Text("Selected effect: ", selected.name);
		
			ImGui::SliderInt("Effect Index", &currentBackgroundEffect,0, backgroundEffects.size() - 1);
		
			ImGui::InputFloat4("data1",(float*)& selected.data.data1);
			ImGui::InputFloat4("data2",(float*)& selected.data.data2);
			ImGui::InputFloat4("data3",(float*)& selected.data.data3);
			ImGui::InputFloat4("data4",(float*)& selected.data.data4);
		}
		ImGui::End();

        //make imgui calculate internal draw structures
        ImGui::Render();

        draw(deltaTime);
    }
}

void VulkanEngine::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
    VkRenderingAttachmentInfo attachmentInfo = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = vkinit::rendering_info(_swapchainExtent, &attachmentInfo, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;
    bufferInfo.pNext = nullptr;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = memoryUsage;
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    AllocatedBuffer buffer;
    VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaAllocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
    return buffer;
}

void VulkanEngine::destroy_buffer(const AllocatedBuffer& buffer)
{
    vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

//> create image
AllocatedImage VulkanEngine::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
    AllocatedImage image;
    image.imageExtent = size;
    image.imageFormat = format;

    VkImageCreateInfo createInfo = vkinit::image_create_info(format, usage, size);
    if (mipmapped)
    {
        createInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height))) + 1);
    }

    // allocated image on dedicated GPU memory
    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    // allocate and create the image
    VK_CHECK(vmaCreateImage(_allocator, &createInfo, &vmaAllocInfo, &image.image, &image.allocation, nullptr));

    // correct aspect flag if format is depth
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT)
    {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build image view
    VkImageViewCreateInfo viewCreateInfo = vkinit::imageview_create_info(format, image.image, aspectFlags);
    viewCreateInfo.subresourceRange.levelCount = createInfo.mipLevels;

    VK_CHECK(vkCreateImageView(_device, &viewCreateInfo, nullptr, &image.imageView));

    return image;
}

AllocatedImage VulkanEngine::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
    // default image format as RGBA8
    size_t data_size = size.width * size.height * 4;
    AllocatedBuffer stagingBuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(stagingBuffer.allocationInfo.pMappedData, data, data_size);

    AllocatedImage image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    immediate_submit([&](VkCommandBuffer cmd)
    {
        vkutil::transition_image(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = size;

        // copy the buffer to image
        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        vkutil::transition_image(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    destroy_buffer(stagingBuffer);
    
    return image;
}

void VulkanEngine::destroy_image(const AllocatedImage& image)
{
    if (image.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, image.imageView, nullptr);
    }
    if (image.image != VK_NULL_HANDLE) {
        vmaDestroyImage(_allocator, image.image, image.allocation);
    }
}
//> create image

GPUMeshBuffers VulkanEngine::uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
    size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers surface;

    // create vertex buffer
    surface.vertexBuffer = create_buffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // find vertex buffer address
    VkBufferDeviceAddressInfo vertexAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = surface.vertexBuffer.buffer};
    surface.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &vertexAddressInfo);

    surface.indexBuffer = create_buffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);


    // need a staging buffer that cpu writable, and execute cpoy command to copy data from staging buffer to gpu buffer
    AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data = staging.allocation->GetMappedData();
    // copy vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // copy index buffer
    memcpy((uint8_t*)data + vertexBufferSize, indices.data(), indexBufferSize);

    immediate_submit([&](VkCommandBuffer cmd)
    {
        VkBufferCopy vertexCopy{};
        vertexCopy.srcOffset = 0;
        vertexCopy.dstOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, surface.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{};
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.dstOffset = 0;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, surface.indexBuffer.buffer, 1, &indexCopy);
    });

    destroy_buffer(staging);

    return surface;
}

void VulkanEngine::resize_swapchain()
{
	vkDeviceWaitIdle(_device);

	destroy_swapchain();

	create_swapchain();

	resize_requested = false;
}

//>MeshNode

void MeshNode::Draw(const glm::mat4& parentMatrix, DrawContext& ctx)
{
    glm::mat4 nodeMatrix = parentMatrix * worldTransform;

    for (auto &surface : mesh->surfaces)
    {
        RenderObject obj;
        obj.firstIndex = surface.startIndex;
        obj.indexCount = surface.count;
        obj.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
        obj.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

        obj.transform = nodeMatrix;
        obj.material = &surface.material->data;

        ctx.opaqueSurface.push_back(obj);
    }

    Node::Draw(parentMatrix, ctx);
}

//>MeshNode
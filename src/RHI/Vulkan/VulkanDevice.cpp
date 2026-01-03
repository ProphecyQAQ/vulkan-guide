#include <Vulkan/VulkanDevice.h>
#include <Vulkan/VulkanHelper.h>
#include <cstdint>
#include <vector>
#include <Core/Log.h>

static inline std::string GetQueueInfoString(const VkQueueFamilyProperties& props)
{
	std::string info;
	if ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
	{
		info += TEXT(" Gfx");
	}
	if ((props.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT)
	{
		info += TEXT(" Compute");
	}
	if ((props.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
	{
		info += TEXT(" Xfer");
	}
	if ((props.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == VK_QUEUE_SPARSE_BINDING_BIT)
	{
		info += TEXT(" Sparse");
	}

	return info;
};

bool VulkanDevice::checkValidationSupport()
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

VulkanDevice::VulkanDevice(VkInstance instance)
{
    deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    validationLayer = {
        "VK_LAYER_KHRONOS_validation"
    };

    graphicsQueueFamilyIndex = -1;
    computeQueueFamilyIndex = -1;
    transferQueueFamilyIndex = -1;

    chosenGPU = selectPhysicalDevice(instance);
    createDevice();
}

VkPhysicalDevice VulkanDevice::selectPhysicalDevice(VkInstance instance)
{
    std::vector<VkPhysicalDevice> physicalDevices = VulkanHeaplerLibrary::getPhysicalDevices(instance);

    std::vector<VkPhysicalDeviceProperties> deviceProperties;
    for (const auto& phyDev : physicalDevices)
    {
        VkPhysicalDeviceProperties deviceProp;
        vkGetPhysicalDeviceProperties(phyDev, &deviceProp);
        deviceProperties.push_back(deviceProp);
    }

    std::sort(deviceProperties.begin(), deviceProperties.end(), [](const VkPhysicalDeviceProperties& lhs, const VkPhysicalDeviceProperties& rhs) 
    {
        return (lhs.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) || (rhs.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU);
    });

    return physicalDevices[0];
}

void VulkanDevice::createDevice()
{
    std::vector<VkQueueFamilyProperties> queueFamilyProperties  = VulkanHeaplerLibrary::getQueueFamily(chosenGPU);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    int32_t queueNum = 0;
    for (int32_t familyIndex = 0; familyIndex < queueFamilyProperties.size(); familyIndex ++)
    {
        const VkQueueFamilyProperties& currProps = queueFamilyProperties[familyIndex];
        
        bool bValidQueue = false;
        if (currProps.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            if (graphicsQueueFamilyIndex == -1)
            {
                graphicsQueueFamilyIndex = familyIndex;
                bValidQueue = true;
            }
        }

        if (currProps.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            if (computeQueueFamilyIndex == -1 && graphicsQueueFamilyIndex != familyIndex)
            {
                computeQueueFamilyIndex = familyIndex;
                bValidQueue = true;
            }
        }

        if (currProps.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            if (transferQueueFamilyIndex == -1)
            {
                transferQueueFamilyIndex = familyIndex;
                bValidQueue = true;
            }
        }

        if (!bValidQueue)
        {
            LOG_INFO("Queue family {} is not suitable for graphics, compute or transfer", familyIndex);
            continue;
        }

        VkDeviceQueueCreateInfo queueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = static_cast<uint32_t>(familyIndex),
            .queueCount = currProps.queueCount
        };
        queueCreateInfos.push_back(queueCreateInfo);
        queueNum += currProps.queueCount;
        LOG_INFO("Initialize queue family {}: {}, queues {}", familyIndex, currProps.queueCount, GetQueueInfoString(currProps));
    }

    std::vector<float> queuePriority(queueNum, 1.0f);
    float* currPriority = queuePriority.data();
    for (VkDeviceQueueCreateInfo& queueCreateInfo : queueCreateInfos)
    {
        queueCreateInfo.pQueuePriorities = currPriority;
        currPriority += queueCreateInfo.queueCount;
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
    // enable multi draw indirect
    deviceFeatures.multiDrawIndirect = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.pNext = &sync2Features;

    if (checkValidationSupport())
    {
        deviceCreateInfo.enabledLayerCount = 1;
        deviceCreateInfo.ppEnabledLayerNames = validationLayer.data();
    }
    else 
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(chosenGPU, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("[VulkanEngine] [init_valkan] failed to create logical device!");
    }

    // get queue
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    if (computeQueueFamilyIndex != -1)
    {
        vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);
    }
    if (transferQueueFamilyIndex != -1)
    {
        vkGetDeviceQueue(device, transferQueueFamilyIndex, 0, &transferQueue);
    }   
}

void VulkanDevice::setPresnentQueue(VkSurfaceKHR surface)
{
    static bool bSetPresentQueue = false;

    if (!bSetPresentQueue)
    {
        // check graphics queue family support present
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(chosenGPU, graphicsQueueFamilyIndex, surface, &presentSupport);
        if (presentSupport)
        {
            LOG_INFO("Set present queue to graphics queue");
            bSetPresentQueue = true;
        }
        else
        {
            throw std::runtime_error("[VulkanDevice] [setPresnentQueue] graphics queue does not support present!");
        }
    }
}
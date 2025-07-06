// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_healper.h>

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function)
	{
		deletors.push_back(function);
	}

	void flush()
	{
		for (auto it = deletors.rbegin(); it != deletors.rend(); it ++)
		{
			(*it)();
		}
		deletors.clear();
	}
};

struct FrameData {
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;

	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;

	DeletionQueue _deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	static VulkanEngine& Get();

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//draw image background
	void draw_background(VkCommandBuffer cmd);

	//run main loop
	void run();

public:
	FrameData& get_current_frame() {return _frameData[_frameNumber%FRAME_OVERLAP];}

public:
	std::vector<const char*> _validationLayer;
	std::vector<const char*> _deviceExtensions;

	VmaAllocator _allocator;

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messager; // Vulkan debug output handle
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkQueue _graphicsQueue;

	FrameData _frameData[FRAME_OVERLAP];

	// presentation
	VkSurfaceKHR _surface;
	VkQueue _presentQueue;
	VkSwapchainKHR _swapChain;
	std::vector<VkImage> _swapChainImage;
	std::vector<VkImageView> _swapChainImageView;
	VkFormat _swapChainFormat;
	VkExtent2D _swapChainExtent;

	// draw resource
	AllocatedImage _drawImage;
	VkExtent2D _drawExtent;

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	bool check_validation_support();
	QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
	bool is_device_suitable(VkPhysicalDevice physicalDevice);
	bool is_queue_family_suitable_for_graphics(VkQueueFamilyProperties queueFamilyProperty);
	bool is_queue_family_suitable_for_presentation(VkQueueFamilyProperties queueFamilyProperty, uint32_t queueFamilyIndex);
	
	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	DeletionQueue _mainDeletionQueue;
};

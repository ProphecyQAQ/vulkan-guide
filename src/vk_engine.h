// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_descriptors.h>
#include <vk_healper.h>
#include <vk_loader.h>
#include <vk_pipelines.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
    const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
};

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
	DescriptorAllocatorGrowable _frameDescriptors;
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

	// draw gemotroy
	void draw_geometry(VkCommandBuffer cmd);

	//run main loop
	void run();

public:
	FrameData& get_current_frame() {return _frameData[_frameNumber%FRAME_OVERLAP];}
	
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
	GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
public:
	std::vector<const char*> _validationLayer;
	std::vector<const char*> _deviceExtensions;

	// descriptor
	DescriptorAllocator globalDescriptorAllocator;

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
	VkSwapchainKHR _swapchain;
	std::vector<VkImage> _swapchainImage;
	std::vector<VkImageView> _swapchainImageView;
	VkFormat _swapchainImageFormat;
	VkExtent2D _swapchainExtent;

	// draw resource
	AllocatedImage _drawImage;
	AllocatedImage _depthImage;
	VkExtent2D _drawExtent;
	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	// Pipeline
	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;
	VkPipeline _meshPipeline;
	VkPipelineLayout _meshPipelineLayout;

	// immediate submit structures
	VkFence _immFence;
	VkCommandPool _immCommandPool;
	VkCommandBuffer _immCommandBuffer;

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();
	// descriptor
	void init_descriptors();
	// pipelines
	void init_pipelines();
	void init_background_pipelines();
	void init_triangle_pipeline();
	void init_mesh_pipeline();
	// ui
	void init_imgui();
	// default data
	void init_default_data();

	bool check_validation_support();
	QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
	bool is_device_suitable(VkPhysicalDevice physicalDevice);
	bool is_queue_family_suitable_for_graphics(VkQueueFamilyProperties queueFamilyProperty);
	bool is_queue_family_suitable_for_presentation(VkQueueFamilyProperties queueFamilyProperty, uint32_t queueFamilyIndex);
	
	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

	void create_swapchain();
	void destroy_swapchain();
	void resize_swapchain();
	/**
	* @PARAM allocSize: size of the buffer to allocate
	* @PARAM usage: usage flags for the buffer
	* @PARAM memoryUsage: control where VMA will put our buffer
	*/
	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroy_buffer(const AllocatedBuffer& buffer);
private:
	GPUSceneData _sceneData;
	VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

	DeletionQueue _mainDeletionQueue;

	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{0};

	std::vector<std::shared_ptr<MeshAsset>> testMeshes;
	bool resize_requested;
};

// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_descriptors.h>
#include <vk_healper.h>
#include <vk_loader.h>
#include <vk_pipelines.h>
#include <vk_material.h>
#include <camera.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

struct EngineStats {
	float frametime;
	int triangle_count;
	int drawcall_count;
	float scene_update_time;
	float mesh_draw_time;
};

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

//>RenderObject
struct RenderObject {
	uint32_t indexCount;
	uint32_t firstIndex;
	VkBuffer indexBuffer;

	MaterialInstance* material;
	Bounds bounds;

	glm::mat4 transform;
	VkDeviceAddress vertexBufferAddress;
};

struct DrawContext {
	std::vector<RenderObject> opaqueSurface;
};

struct MeshNode : public Node {
	std::shared_ptr<MeshAsset> mesh;
	virtual void Draw(const glm::mat4& parentMatrix, DrawContext& ctx) override;
};
//>RenderObject

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
	void draw(float deltaTime);

	//draw image background
	void draw_background(VkCommandBuffer cmd);

	// draw gemotroy
	void draw_geometry(VkCommandBuffer cmd);

	//run main loop
	void run();

	// update draw context
	void update_scene(float deltaTime);
public:
	FrameData& get_current_frame() {return _frameData[_frameNumber%FRAME_OVERLAP];}
	
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
	GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

	VkDevice get_device() const { return _device; }
	VkDescriptorSetLayout get_gpu_scene_data_descriptor_layout() const { return _gpuSceneDataDescriptorLayout; }
public:	
	std::vector<const char*> _validationLayer;
	std::vector<const char*> _deviceExtensions;

	// descriptor
	DescriptorAllocatorGrowable globalDescriptorAllocator;

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
	VkDescriptorSetLayout _singleImageDescriptorLayout;

	// Pipeline
	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;

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
public:
	/**
	* @PARAM allocSize: size of the buffer to allocate
	* @PARAM usage: usage flags for the buffer
	* @PARAM memoryUsage: control where VMA will put our buffer
	*/
	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroy_buffer(const AllocatedBuffer& buffer);

	AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage create_image(void *data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void destroy_image(const AllocatedImage& image);
public:
 	std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;
	
	// default image
	AllocatedImage _whiteImage;
	AllocatedImage _blackImage;
	AllocatedImage _greyImage;
	AllocatedImage _errorCheckerboardImage;

	VkSampler _defaultSamplerLinear;
	VkSampler _defaultSamplerNearest;

	// camera
	Camera _mainCamera;

	// draw context
	DrawContext _mainDrawContext;
	std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

	// default material
	GLTFMetallic_Roughness _metalRoughMaterial;

	GPUSceneData _sceneData;
	VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

	DeletionQueue _mainDeletionQueue;

	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{0};

	std::vector<std::shared_ptr<MeshAsset>> testMeshes;
	bool resize_requested;

	// stat
	EngineStats stats;
};

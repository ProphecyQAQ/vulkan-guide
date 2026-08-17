#pragma once

#include <Core/Window.h>
#include <RHI/GraphicsContext.h>
#include <Vulkan/VulkanPCH.h>
#include <Vulkan/VulkanSwapChain.h>
#include <Vulkan/VulkanDevice.h>
#include <Vulkan/VulkanCommandBuffer.h>
#include <Vulkan/VulkanDescriptorSet.h>
#include <Vulkan/VulkanPipeline.h>
#include <Vulkan/VulkanImage.h>
#include <Vulkan/VulkanBuffer.h>

struct RenderObject
{
	RenderObject() = default;
	~RenderObject() 
	{
		if (vertexBuffer)
		{
			delete vertexBuffer;
		}
		if (indexBuffer)
		{
			delete indexBuffer;
		}
	}	

	RenderObject(const RenderObject&) = delete;
	RenderObject& operator=(const RenderObject&) = delete;

	RenderObject(RenderObject&& other) noexcept
	{
		vertexBuffer = other.vertexBuffer;
		indexBuffer = other.indexBuffer;
		indexCount = other.indexCount;
		firstIndex = other.firstIndex;
		transform = other.transform;

		other.vertexBuffer = nullptr;
		other.indexBuffer = nullptr;
	}

	VulkanBuffer* vertexBuffer;
	VulkanBuffer* indexBuffer;
	uint32_t indexCount;
	uint32_t firstIndex;
	glm::mat4 transform;
};

struct FrameContext
{
	FrameContext(VulkanDevice& device);
	~FrameContext();

	VulkanDevice& device;

	VulkanCommandBufferPool* commandBufferPool;
	VulkanCommandBuffer* commandBuffer;

	VulkanDescriptorPoolSet* frameDescriptorPoolSet;

	VkFence renderFence;
	VkSemaphore swapchainSemaphore, renderSemaphore;
};

class VulkanContext : public GraphicsContext
{
public:
    virtual ~VulkanContext();

	static VulkanContext* get();

    virtual void init(Window* window) override;
	virtual void beginFrame() override;
	virtual void drawFrame() override;
	virtual void endFrame() override;
	virtual void submit(RenderData& renderData) override;
	QueueFamilyIndices getQueueFamilyIndices() const { return queueFamilyIndices; }
	VmaAllocator getAllocator() const { return allocator; }
	VkDevice getVkDevice() const { return vulkanDevice->getDevice(); }
	VulkanDevice* getDevice() const { return vulkanDevice; }
private:
	void initImmediateCtx();
	void initFrameContext();

	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func);
	FrameContext* getCurrentFrameContext() { return frameContexts[frameCount % FRAME_OVERLAP]; }
private:
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessager; // Vulkan debug output handle

	VulkanDevice *vulkanDevice;

	std::vector<const char*> deviceExtensions;

	QueueFamilyIndices queueFamilyIndices;

	VulkanSwapChain *swapChain;
	VkSurfaceKHR surface;

	VmaAllocator allocator;

	// draw ctx
	VulkanCommandBufferPool *immediateCmdPool;
	VulkanCommandBuffer* immediateCmdBuffer;
	VkFence immediateFence;

	std::vector<FrameContext*> frameContexts;

	VulkanDescriptorPoolSet* globalDescriptorPoolSet;

	VulkanImage *renderImage;
	VulkanImage *depthImage;

	std::vector<RenderObject> frameRenderDatas;
	uint32_t frameCount = 0;

	// pipeline library
	VulkanPipelineLibrary* pipelineLibrary;
private:
	// blow is for test

	// here is compute pipeline
	void initComputePipeline();
	void drawComputePipeline(VkCommandBuffer cmd);
	struct ComputePipelinePushConstantData 
	{
		glm::vec4 data1;
		glm::vec4 data2;
		glm::vec4 data3;
		glm::vec4 data4;
	} ComputePipelinePushConstantData;

	// here is unlit pipeline
	struct UnlitPipelinePushConstantData 
	{
		glm::mat4 render_matrix;
		VkDeviceAddress vertexBufferAddress;
	} unlitPipelinePushConstantData;
	void initUnlitPipeline();
	void drawUnlitPipeline(VkCommandBuffer cmd);

	struct SceneData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewproj;
	} sceneData;

	VulkanBuffer* SceneDataBuffer;
};
#pragma once

#include <Vulkan/VulkanPCH.h>
#include <Vulkan/VulkanDevice.h>

class VulkanDescriptorSetLayout
{
public:
    struct Builder
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings; 

        void addBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount);

        VulkanDescriptorSetLayout* build(VkDevice device, VkShaderStageFlags stageFlags, VkDescriptorSetLayoutCreateFlags flags = 0);
    };
    VulkanDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);

    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
private:
    VkDevice device;
    VkDescriptorSetLayout descriptorSetLayout;
};

class VulkanDescriptorSet
{
public:
    struct Writer
    {
        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        void writeImage(int binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
        void writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

        void clear();
        void update(VulkanDevice& device, VkDescriptorSet descriptorSet);
    };

    VulkanDescriptorSet(VulkanDevice& device, VulkanDescriptorSetLayout& setLayout);
    ~VulkanDescriptorSet();
private:
    VulkanDevice& device;
    VulkanDescriptorSetLayout& setLayout;
};

class VulkanDescriptorPool
{
public:
    VulkanDescriptorPool(VulkanDevice& device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes);
    ~VulkanDescriptorPool();

    VkDescriptorPool getDescriptorPool() const { return descriptorPool; }
    VkResult allocateDescriptorSet(VulkanDescriptorSetLayout* setLayout, VkDescriptorSet& outSet);
private:
    VulkanDevice& device;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
};

class VulkanDescriptorPoolSet
{
public:
    VulkanDescriptorPoolSet(VulkanDevice& device);
    ~VulkanDescriptorPoolSet();

    VkDescriptorSet allocateDescriptorSet(VulkanDescriptorSetLayout* setLayout);
    void clear();
private:
    VulkanDescriptorPool* createNewPool();
    VulkanDescriptorPool* getFreePool();
private:
    VulkanDevice& device;
    std::vector<VulkanDescriptorPool*> descriptorPools;

    uint32_t currentPoolIndex = -1;
};
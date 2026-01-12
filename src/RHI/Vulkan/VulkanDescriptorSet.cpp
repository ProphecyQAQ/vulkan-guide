#include "glm/common.hpp"
#include <Vulkan/VulkanDescriptorSet.h>
#include <cstdint>

// -------------VulkanDescriptorSetLayout----------------
void VulkanDescriptorSetLayout::Builder::addBinding(uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.descriptorCount = descriptorCount;
    layoutBinding.stageFlags = VK_SHADER_STAGE_ALL; // For simplicity, using all shader stages

    bindings.push_back(layoutBinding);
}

VulkanDescriptorSetLayout* VulkanDescriptorSetLayout::Builder::build(VkDevice device, VkShaderStageFlags stageFlages, VkDescriptorSetLayoutCreateFlags flags /* = 0*/)
{
    for (VkDescriptorSetLayoutBinding& binding : bindings)
    {
        binding.stageFlags |= stageFlages;
    }

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.pBindings = this->bindings.data();
    createInfo.bindingCount = static_cast<uint32_t>(this->bindings.size());
    createInfo.flags = flags;

    VkDescriptorSetLayout setLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &setLayout));

    return new VulkanDescriptorSetLayout(device, setLayout);
} 

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
    : device(device), descriptorSetLayout(descriptorSetLayout)
{
}

// -------------VulkanDescriptorSetLayout----------------

// -------------VulkanDescriptorSet----------------
VulkanDescriptorSet::VulkanDescriptorSet(VulkanDevice& device, VulkanDescriptorSetLayout& setLayout)
    : device(device), setLayout(setLayout)
{
    
}
// -------------VulkanDescriptorSet----------------

// -------------VulkanDescriptorPool----------------
VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice& device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes)
    : device(device), descriptorPoolSizes(poolSizes)
{
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    createInfo.pPoolSizes = descriptorPoolSizes.data();
    createInfo.maxSets = maxSets;

    VK_CHECK(vkCreateDescriptorPool(device.getDevice(), &createInfo, nullptr, &descriptorPool));
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
    vkDestroyDescriptorPool(device.getDevice(), descriptorPool, nullptr);
}

VkResult VulkanDescriptorPool::allocateDescriptorSet(VulkanDescriptorSetLayout* setLayout, VkDescriptorSet& outSet)
{
    VkDescriptorSetLayout layout = setLayout->getDescriptorSetLayout();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.pSetLayouts = &layout;
    allocInfo.descriptorSetCount = 1;

    VkResult result = vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &outSet);

    return result;
}

// -------------VulkanDescriptorPool----------------

// -------------VulkanDescriptorPoolSet----------------

VulkanDescriptorPoolSet::VulkanDescriptorPoolSet(VulkanDevice& device)
    : device(device)
{
    createNewPool();
}

VulkanDescriptorPoolSet::~VulkanDescriptorPoolSet()
{
    for (VulkanDescriptorPool* pool : descriptorPools)
    {
        delete pool;
    }
}

VulkanDescriptorPool* VulkanDescriptorPoolSet::createNewPool()
{
    const uint32_t maxSetsBase = 32;
    const uint32_t maxSets = maxSetsBase << glm::min(2u, static_cast<uint32_t>(descriptorPools.size()));

    // For simplicity, using fixed pool sizes
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 * maxSets },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 * maxSets },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 * maxSets },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * maxSets },
    };

    VulkanDescriptorPool* newPool = new VulkanDescriptorPool(device, maxSets, poolSizes);

    descriptorPools.push_back(newPool);
    currentPoolIndex += 1;

    return newPool;
}

VulkanDescriptorPool* VulkanDescriptorPoolSet::getFreePool()
{
    while (currentPoolIndex < descriptorPools.size())
    {
        return descriptorPools[currentPoolIndex];
    }

    return createNewPool();
}


VkDescriptorSet VulkanDescriptorPoolSet::allocateDescriptorSet(VulkanDescriptorSetLayout* setLayout)
{
    if (descriptorPools.empty())
    {
        createNewPool();
    }

    VkDescriptorSet outSet;

    VulkanDescriptorPool* pool = descriptorPools[currentPoolIndex];
    while (pool->allocateDescriptorSet(setLayout, outSet) != VK_SUCCESS)
    {
        pool = getFreePool();
    }

    return outSet;
}
// -------------VulkanDescriptorPoolSet----------------
#include <vk_descriptors.h>

//>DescriptorLayoutBuilder
void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding = binding;
    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;

    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::clear()
{
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext /*= nullptr*/, VkDescriptorSetLayoutCreateFlags flags /*= 0*/)
{
    for (auto &b:bindings)
    {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = pNext;
    createInfo.pBindings = this->bindings.data();
    createInfo.bindingCount = this->bindings.size();
    createInfo.flags = flags;

    VkDescriptorSetLayout setLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &setLayout));
    return setLayout;
}
//>DescriptorLayoutBuilder

//>DescriptorAllocator
void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (auto poolRatio:poolRatios)
    {
        poolSizes.push_back(
            VkDescriptorPoolSize{
                .type = poolRatio.type,
                .descriptorCount = uint32_t(poolRatio.ratio * maxSets)
            }
        );
    }

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.flags = 0;
    createInfo.maxSets = maxSets;
    createInfo.poolSizeCount = poolSizes.size();
    createInfo.pPoolSizes = poolSizes.data();

    VK_CHECK(vkCreateDescriptorPool(device, &createInfo, nullptr, &pool));
}

void DescriptorAllocator::clear_descriptors(VkDevice device)
{
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::destroy_pool(VkDevice device)
{
    vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, nullptr));

    return descriptorSet;
}
//DescriptorAllocator
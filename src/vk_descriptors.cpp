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
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.maxSets = maxSets;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
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

    VkDescriptorSet descriptorSet{};
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

    return descriptorSet;
}
//DescriptorAllocator

//>DescriptorAllocatorGrowable

void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
    ratios.clear();
    for (auto ratio:poolRatios)
    {
        ratios.push_back(ratio);
    }

    setsPerPool = maxSets * 1.5;

    VkDescriptorPool pool = create_pool(device, maxSets, poolRatios);
    readyPools.push_back(pool);
}

void DescriptorAllocatorGrowable::clear_pools(VkDevice device)
{
    for (auto pool : readyPools)
    {
        vkResetDescriptorPool(device, pool, 0);
    }
    for (auto pool : fullPools)
    {
        vkResetDescriptorPool(device, pool, 0);
        readyPools.push_back(pool);
    }
    fullPools.clear();
}

void DescriptorAllocatorGrowable::destroy_pools(VkDevice device)
{
    for (auto pool:readyPools)
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    for (auto pool:fullPools)
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    readyPools.clear();
    fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
    // get pool
    VkDescriptorPool pool = get_pool(device);

    VkDescriptorSetAllocateInfo allocaInfo{};
    allocaInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocaInfo.pSetLayouts = &layout;
    allocaInfo.descriptorPool = pool;
    allocaInfo.descriptorSetCount = 1;
    allocaInfo.pNext = pNext;

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &allocaInfo, &ds);

    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        fullPools.push_back(pool);
        pool = get_pool(device);
        VK_CHECK( vkAllocateDescriptorSets(device, &allocaInfo, &ds));
    }
    readyPools.push_back(pool);
    return ds;
}

VkDescriptorPool DescriptorAllocatorGrowable::create_pool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSize;
    for (auto poolSizeRatio:poolRatios)
    {
        poolSize.push_back(
            VkDescriptorPoolSize{
                .type = poolSizeRatio.type,
                .descriptorCount = uint32_t(poolSizeRatio.ratio * setCount)
            }
        );
    }

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
    createInfo.pPoolSizes = poolSize.data();
    createInfo.maxSets = setCount;
    createInfo.pNext = nullptr;

    VkDescriptorPool pool;
    VK_CHECK(vkCreateDescriptorPool(device, &createInfo, nullptr, &pool));
    return pool;
}

VkDescriptorPool DescriptorAllocatorGrowable::get_pool(VkDevice device)
{
    VkDescriptorPool pool;
    if (readyPools.size() > 0)
    {
        pool = readyPools.back();
        readyPools.pop_back();
    }
    else
    {
        pool = create_pool(device, setsPerPool, ratios);

        setsPerPool *= 1.5;
        setsPerPool = std::min((uint32_t)4092, setsPerPool);
    }
    return pool;
}

//>DescriptorAllocatorGrowable


//>DescriptorWriter

void DescriptorWriter::write_image(int binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    VkDescriptorImageInfo &info = imageInfos.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = imageView,
        .imageLayout = layout,
    });

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // This will be set later when the writer is used
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;
    write.pNext = nullptr;
    writes.push_back(write);
}

void DescriptorWriter::write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = size;
    bufferInfos.emplace_back(bufferInfo);

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // This will be set later when the writer is used
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &bufferInfo;
    write.pNext = nullptr;

    writes.push_back(write);
}

void DescriptorWriter::clear()
{
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
}

void DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
{
    for (auto& write : writes)
    {
        write.dstSet = set; // Set the destination set for each write
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

//>DescriptorWriter
#include <vk_descriptors.h>

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
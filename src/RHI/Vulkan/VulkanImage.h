#pragma once

#include <Vulkan/VulkanPCH.h>

class VulkanImage
{
public:
    VulkanImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
    ~VulkanImage();

    VkImage getImage() { return image; }
    VkImageView getImageView() const { return imageView; }
    VkFormat getFormat() const { return format; }
private:
    VkImageCreateInfo imageCreateInfo();
    VkImageViewCreateInfo imageviewCreateInfo(VkImageAspectFlags aspectFlags);
private:
    VkImage image;
    VmaAllocation allocation;
    VkImageView imageView;
    VkFormat format;
    VkExtent3D size;
    VkImageUsageFlags usageFlags;
    bool mipmapped;
};
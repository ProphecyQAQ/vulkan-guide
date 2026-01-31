#include <Vulkan/VulkanImage.h>
#include <Vulkan/VulkanContext.h>

VulkanImage::VulkanImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
    : format(format), size(size), usageFlags(usage), mipmapped(mipmapped)
{
    // create image
    VkImageCreateInfo imageInfo = imageCreateInfo();

    // allocated image on dedicated GPU memory
    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK(vmaCreateImage(VulkanContext::get()->getAllocator(), &imageInfo, &vmaAllocInfo, &image, &allocation, nullptr));

    // correct aspect flag if format is depth
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT)
    {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build image view
    VkImageViewCreateInfo viewCreateInfo = imageviewCreateInfo(aspectFlags);
    viewCreateInfo.subresourceRange.levelCount = imageInfo.mipLevels;

    VK_CHECK(vkCreateImageView(VulkanContext::get()->getVkDevice(), &viewCreateInfo, nullptr, &imageView));
}

VulkanImage::~VulkanImage()
{
    vmaDestroyImage(VulkanContext::get()->getAllocator(), image, allocation);
    vkDestroyImageView(VulkanContext::get()->getVkDevice(), imageView, nullptr);
}

VkImageCreateInfo VulkanImage::imageCreateInfo()
{
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;

    info.imageType = VK_IMAGE_TYPE_2D;

    info.format = format;
    info.extent = size;

    info.mipLevels = 1;
    if (mipmapped)
    {
        info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
    }
    info.arrayLayers = 1;

    //for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
    info.samples = VK_SAMPLE_COUNT_1_BIT;

    //optimal tiling, which means the image is stored on the best gpu format
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usageFlags;

    return info;
}

VkImageViewCreateInfo VulkanImage::imageviewCreateInfo(VkImageAspectFlags aspectFlags)
{
    // build a image-view for the depth image to use for rendering
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;

    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = image;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectFlags;

    return info;
}
#include <RHI/GraphicsContext.h>
#include <Vulkan/VulkanContext.h>

std::unique_ptr<GraphicsContext> GraphicsContext::create()
{
    return std::make_unique<VulkanContext>();
}
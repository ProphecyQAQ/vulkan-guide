#include <RHI/GraphicsContext.h>
#include <Vulkan/VulkanContext.h>

RenderSystem::RenderSystem(Window* window)
{
    graphicsContext = std::make_unique<VulkanContext>();
    graphicsContext->init(window);
}

void RenderSystem::onUpdate()
{
    graphicsContext->beginFrame();
    graphicsContext->drawFrame();
    graphicsContext->endFrame();
}
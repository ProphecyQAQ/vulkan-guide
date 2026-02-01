#include <RHI/GraphicsContext.h>
#include <Vulkan/VulkanContext.h>

RenderSystem* RenderSystem::renderSystem = nullptr;

RenderSystem* RenderSystem::get()
{
    return renderSystem;
}

RenderSystem::RenderSystem(Window* window)
{
    renderSystem = this;
    
    graphicsContext = std::make_unique<VulkanContext>();
    graphicsContext->init(window);
}

void RenderSystem::onUpdate()
{
    graphicsContext->beginFrame();
    graphicsContext->drawFrame();
    graphicsContext->endFrame();
}

void RenderSystem::submit(RenderData& renderData)
{
    // Forward to graphics context or store for rendering
    graphicsContext->submit(renderData);
}
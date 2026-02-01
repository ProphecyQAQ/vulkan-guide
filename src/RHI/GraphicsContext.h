#pragma once

#include <memory>
#include <Core/BaseType.h>

class Window;
class GraphicsContext
{
public:
    virtual ~GraphicsContext() = default;

    virtual void init(Window* window) = 0;

	virtual void beginFrame() = 0;
	virtual void drawFrame() = 0;
	virtual void endFrame() = 0;

    virtual void submit(RenderData& renderData) = 0;
};

class RenderSystem
{
public:
    static RenderSystem* get();

    RenderSystem(Window* window);
    virtual ~RenderSystem() = default;

    void submit(RenderData& renderData);
    void onUpdate();
private:
    std::unique_ptr<GraphicsContext> graphicsContext;

    static RenderSystem* renderSystem;
};
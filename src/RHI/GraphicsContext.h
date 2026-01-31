#pragma once

#include <memory>

class Window;
class GraphicsContext
{
public:
    virtual ~GraphicsContext() = default;

    virtual void init(Window* window) = 0;

	virtual void beginFrame() = 0;
	virtual void drawFrame() = 0;
	virtual void endFrame() = 0;
};

class RenderSystem
{
public:
    RenderSystem(Window* window);
    virtual ~RenderSystem() = default;

    void onUpdate();
private:
    std::unique_ptr<GraphicsContext> graphicsContext;
};
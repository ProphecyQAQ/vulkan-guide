#pragma once

#include <memory>

class Window;
class GraphicsContext
{
public:
    virtual ~GraphicsContext() = default;

    virtual void init(Window* window) = 0;

    static std::unique_ptr<GraphicsContext> create();
};
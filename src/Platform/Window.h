#pragma once

#include <core/Window.h>
#include <memory>

class Window : public IWindow
{
public:
    virtual ~Window() = default;

    static std::unique_ptr<Window> create(const WindowProps& props = WindowProps());
};
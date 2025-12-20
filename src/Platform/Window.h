#pragma once

#include <core/Window.h>

class Window : public IWindow
{
public:
    virtual ~Window() = default;

    static Window* create(const WindowProps& props = WindowProps());
};
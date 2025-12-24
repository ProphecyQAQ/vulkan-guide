#pragma once

#include <core/Window.h>
#include <pch.h>

class Window : public IWindow
{
public:
    virtual ~Window() = default;

    virtual std::vector<const char*> getVulkanExtensions() const;

    static std::unique_ptr<Window> create(const WindowProps& props = WindowProps());
};
#pragma once

#include <string>
#include <memory>
#include <vector>

struct WindowProps
{
    WindowProps(const std::string& title = "Hazel Engine",
                uint32_t width = 1600,
                uint32_t height = 900)
        : Title(title), Width(width), Height(height){}

    std::string Title;
    uint32_t Width;
    uint32_t Height;
};

class Window
{
public:
    virtual ~Window() = default;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual std::string getTitle() const = 0;

    virtual void* getNativeWindow() const = 0; 

    virtual void onUpdate() = 0;

    virtual std::vector<const char*> getVulkanExtensions() const = 0;

    static std::unique_ptr<Window> create(const WindowProps& props = WindowProps());
};
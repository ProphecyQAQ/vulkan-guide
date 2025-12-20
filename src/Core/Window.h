#pragma once

#include <string>

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

class IWindow
{
public:
    virtual ~IWindow() = default;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual std::string getTitle() const = 0;

    virtual void onUpdate() = 0;
};
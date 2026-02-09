#pragma once

#include <Core/Window.h>
#include <vector>

struct SDL_Window;

class WindowsWindow : public Window
{
public:
    WindowsWindow(const WindowProps& props);
    virtual ~WindowsWindow();

    virtual uint32_t getWidth() const override {
        return props.Width;
    }

    virtual uint32_t getHeight() const override {
        return props.Height;
    }

    virtual std::string getTitle() const override
    {
        return props.Title;
    }

    virtual void* getNativeWindow() const override {
        return reinterpret_cast<void*>(window);
    } 

    virtual std::vector<const char*> getVulkanExtensions() const override;

    virtual void onUpdate() override;

    virtual bool isExit() override { return bQuit; }
private:
    SDL_Window* window;
    WindowProps props;
    bool bQuit = false;
};
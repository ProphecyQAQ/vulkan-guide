#pragma once

#include <Platform/Window.h>
#include <SDL.h>
#include <vector>

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

    virtual void onUpdate() override;
private:
    SDL_Window* window;
    WindowProps props;
    bool bQuit = false;
};
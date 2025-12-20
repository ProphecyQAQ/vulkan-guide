#pragma once

#include <Window.h>
#include <SDL.h>

class WindowsWindow : public Window
{
public:
    WindowsWindow(const WindowProps& props);
    virtual ~WindowsWindow() = default;

    virtual uint32_t getWidth() const {
        return props.Width;
    }

    virtual uint32_t getHeight() const {
        return props.Height;
    }
    
    virtual std::string getTitle() const
    {
        return props.Title;
    }

private:
    SDL_Window* window;
    WindowProps props;
};
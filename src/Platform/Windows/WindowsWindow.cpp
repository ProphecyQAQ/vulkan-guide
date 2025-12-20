#include <Windows/WindowsWindow.h>

WindowsWindow::WindowsWindow(const WindowProps& props)
    :props(props)
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    window = SDL_CreateWindow(
        props.Title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        props.Width,
        props.Height,
        window_flags
    );
}
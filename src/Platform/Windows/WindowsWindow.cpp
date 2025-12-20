#include <Windows/WindowsWindow.h>

WindowsWindow::WindowsWindow(const WindowProps& props)
    :props(props)
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    window = SDL_CreateWindow(
        props.Title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        props.Width,
        props.Height,
        windowFlags
    );

    bQuit = false;
}

WindowsWindow::~WindowsWindow()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void WindowsWindow::onUpdate()
{
    SDL_Event event;

    while (!bQuit)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                bQuit = true;
            }
            
        }
        SDL_Delay(16);
    }
}
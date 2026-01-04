#include <Core/Log.h>
#include <Platform/Windows/WindowsWindow.h>
#include <SDL.h>
#include <SDL_vulkan.h>

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

std::vector<const char*> WindowsWindow::getVulkanExtensions() const
{
    unsigned int sdlExtensionCount = 0;
    std::vector<const char*> sdlExtensions;
    if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, sdlExtensions.data()) == false)
    {
        LOG_WARN("Get instance extestions failed");
        return sdlExtensions;
    }

    sdlExtensions.resize(sdlExtensionCount);

    if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, sdlExtensions.data()) == false)
    {
        LOG_WARN("Get instance extestions failed");
        return sdlExtensions;
    }

    for (unsigned int i = 0; i < sdlExtensionCount; i ++)
    {
        LOG_INFO("SDL Extension: {}", sdlExtensions[i]);
    }

    return sdlExtensions;
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
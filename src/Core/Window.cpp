#include <Window.h>
#include <Platform/Windows/WindowsWindow.h>

std::unique_ptr<Window> Window::create(const WindowProps& props)
{
    return std::make_unique<WindowsWindow>(props);
}
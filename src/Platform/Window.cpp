#include <Window.h>
#include <Windows/WindowsWindow.h>
#include <memory>

std::unique_ptr<Window> Window::create(const WindowProps& props)
{
    return std::make_unique<WindowsWindow>(props);
}
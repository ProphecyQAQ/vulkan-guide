#include <Window.h>
#include <Windows/WindowsWindow.h>

Window* Window::create(const WindowProps& props)
{
    return new WindowsWindow(props);
}
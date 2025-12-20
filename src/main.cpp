#include <Platform/Window.h>

int main()
{
    WindowProps props("My Hazel Window", 1280, 720);
    std::unique_ptr<Window> window = Window::create(props);

    window->onUpdate();

    return 0;
}
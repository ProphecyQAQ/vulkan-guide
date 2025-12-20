#include <Platform/Window.h>
#include <memory>

int main()
{
    WindowProps props("My Hazel Window", 1280, 720);
    std::unique_ptr<Window> window = Window::create(props);

    while (1)
    {
        // Main loop
    }

    return 0;
}
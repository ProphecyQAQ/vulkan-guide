#include <Platform/Window.h>

int main()
{
    WindowProps props("My Hazel Window", 1280, 720);
    Window* window = Window::create(props);

    while (1)
    {
        // Main loop
    }

    // Application loop would go here
    delete window;
    return 0;
}
#include <Platform/Window.h>
#include <Core/Log.h>

int main()
{
    // Init log
    Core::Logger::init();

    WindowProps props("Render Window", 1280, 720);
    std::unique_ptr<Window> window = Window::create(props);

    window->onUpdate();

    return 0;
}
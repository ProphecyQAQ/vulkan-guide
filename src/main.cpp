#include <Core/Window.h>
#include <Core/Log.h>
#include <RHI/GraphicsContext.h>
#include <memory>

int main()
{
    // Init log
    Core::Logger::init();

    WindowProps props("Render Window", 1280, 720);
    std::unique_ptr<Window> window = Window::create(props);

    std::unique_ptr<GraphicsContext> graphicsContext = GraphicsContext::create();
    graphicsContext->init(window.get());

    window->onUpdate();

    return 0;
}
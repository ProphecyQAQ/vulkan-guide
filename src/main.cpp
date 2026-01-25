#include <Core/Window.h>
#include <Core/Log.h>
#include <RHI/GraphicsContext.h>
#include <memory>

int main()
{
    // Init log
    Core::Logger::init();

    WindowProps props("Render Window", 800, 600);
    std::unique_ptr<Window> window = Window::create(props);

    std::unique_ptr<GraphicsContext> graphicsContext = GraphicsContext::create();
    graphicsContext->init(window.get());

    while (true)
    {
        window->onUpdate();
        graphicsContext->beginFrame();
        graphicsContext->drawFrame();
        graphicsContext->endFrame();
    }

    return 0;
}
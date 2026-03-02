#include <Core/Window.h>
#include <Core/Log.h>
#include <Scene/Scene.h>
#include <RHI/GraphicsContext.h>
#include <memory>

int main()
{
    // Init log
    Core::Logger::init();

    WindowProps props("Render Window", 800, 600);
    std::unique_ptr<Window> window = Window::create(props);

    RenderSystem renderSystem(window.get());

    Scene scene;
    Object obj = Object::loadGLTF("D:\\dev\\vulkan-guide1\\assets\\teapot.glb");
    scene.addObject(&obj);

    static std::chrono::high_resolution_clock::time_point lastTickTime = std::chrono::high_resolution_clock::now();

    while (!window->isExit())
    {
        auto currentTickTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = currentTickTime - lastTickTime;

        window->onUpdate();
        scene.OnUpdate(deltaTime.count());
        renderSystem.onUpdate();

        lastTickTime = currentTickTime;
    }

    return 0;
}
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
    // Object obj = Object::loadGLTF("D:\\dev\\vulkan-guide1\\assets\\basicmesh.glb");
    // scene.addObject(&obj);
    while (!window->isExit())
    {
        window->onUpdate();
        scene.OnUpdate();
        renderSystem.onUpdate();
    }

    return 0;
}
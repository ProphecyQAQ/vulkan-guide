#include <Core/Log.h>
#include <Platform/Windows/WindowsWindow.h>
#include <Core/InputSystem.h>
#include <SDL_vulkan.h>

static KeyCode toKeyCode(SDL_Scancode scancode)
{
    switch (scancode)
    {
    case SDL_SCANCODE_W: return KeyCode::W;
    case SDL_SCANCODE_A: return KeyCode::A;
    case SDL_SCANCODE_S: return KeyCode::S;
    case SDL_SCANCODE_D: return KeyCode::D;
    case SDL_SCANCODE_SPACE: return KeyCode::Space;
    case SDL_SCANCODE_ESCAPE: return KeyCode::Escape;
    case SDL_SCANCODE_UP: return KeyCode::Up;
    case SDL_SCANCODE_DOWN: return KeyCode::Down;
    case SDL_SCANCODE_LEFT: return KeyCode::Left;
    case SDL_SCANCODE_RIGHT: return KeyCode::Right;
    default: return KeyCode::Unknown;
    }
}

WindowsWindow::WindowsWindow(const WindowProps& props)
    :props(props)
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    window = SDL_CreateWindow(
        props.Title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        props.Width,
        props.Height,
        windowFlags
    );

    bQuit = false;
}

WindowsWindow::~WindowsWindow()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

std::vector<const char*> WindowsWindow::getVulkanExtensions() const
{
    unsigned int sdlExtensionCount = 0;
    std::vector<const char*> sdlExtensions;
    if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, sdlExtensions.data()) == false)
    {
        LOG_WARN("Get instance extestions failed");
        return sdlExtensions;
    }

    sdlExtensions.resize(sdlExtensionCount);

    if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, sdlExtensions.data()) == false)
    {
        LOG_WARN("Get instance extestions failed");
        return sdlExtensions;
    }

    for (unsigned int i = 0; i < sdlExtensionCount; i ++)
    {
        LOG_INFO("SDL Extension: {}", sdlExtensions[i]);
    }

    return sdlExtensions;
}

void WindowsWindow::processSDLInput(SDL_Event& event)
{
    InputSystem& input = InputSystem::get();
    InputEvent outEvent;
    switch (event.type)
    {
    case SDL_QUIT:
        outEvent.type = InputEvent::Type::Quit;
        input.processEvent(outEvent);
        break;
    case SDL_KEYDOWN:
        outEvent.type = InputEvent::Type::KeyDown;
        outEvent.key = toKeyCode(event.key.keysym.scancode);
        outEvent.repeat = event.key.repeat != 0;
        input.processEvent(outEvent);
        break;
    case SDL_KEYUP:
        outEvent.type = InputEvent::Type::KeyUp;
        outEvent.key = toKeyCode(event.key.keysym.scancode);
        input.processEvent(outEvent);
        break;
    case SDL_MOUSEBUTTONDOWN:
        outEvent.type = InputEvent::Type::MouseButtonDown;
        outEvent.button = static_cast<MouseButton>(event.button.button);
        input.processEvent(outEvent);
        break;
    case SDL_MOUSEBUTTONUP:
        outEvent.type = InputEvent::Type::MouseButtonUp;
        outEvent.button = static_cast<MouseButton>(event.button.button);
        input.processEvent(outEvent);
        break;
    case SDL_MOUSEMOTION:
        outEvent.type = InputEvent::Type::MouseMove;
        outEvent.x = event.motion.x;
        outEvent.y = event.motion.y;
        outEvent.deltaX = event.motion.xrel;
        outEvent.deltaY = event.motion.yrel;
        input.processEvent(outEvent);
        break;
    case SDL_MOUSEWHEEL:
        outEvent.type = InputEvent::Type::MouseWheel;
        outEvent.wheel = event.wheel.y;
        input.processEvent(outEvent);
        break;
    case SDL_TEXTINPUT:
        outEvent.type = InputEvent::Type::TextInput;
        outEvent.text = event.text.text;
        input.processEvent(outEvent);
        break;
    default:
        break;
    }
}

void WindowsWindow::onUpdate()
{
    InputSystem& input = InputSystem::get();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        processSDLInput(event);
    }

    if (input.quitRequested())
    {
        bQuit = true;
    }
    
    SDL_Delay(16);
}

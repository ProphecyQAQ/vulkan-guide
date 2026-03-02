#include <Core/InputSystem.h>
#include <Core/Log.h>

InputSystem& InputSystem::get()
{
    static InputSystem instance;
    return instance;
}

void InputSystem::beginFrame()
{
    keysPrevious = keysCurrent;
    mouseButtonsPrevious = mouseButtonsCurrent;
    mouseWheel = 0;
    text.clear();
    quit = false;
}

static uint32_t buttonMask(MouseButton button)
{
    const uint8_t index = static_cast<uint8_t>(button);
    if (index == 0)
    {
        return 0;
    }
    return 1u << (index - 1);
}

void InputSystem::processEvent(const InputEvent& event)
{
    beginFrame(); // Ensure we start a new frame for each event batch
    switch (event.type)
    {
    case InputEvent::Type::Quit:
        quit = true;
        break;
    case InputEvent::Type::KeyDown:
        if (!event.repeat && static_cast<size_t>(event.key) < KeyCount)
        {
            keysCurrent[static_cast<size_t>(event.key)] = 1;
        }
        break;
    case InputEvent::Type::KeyUp:
        if (static_cast<size_t>(event.key) < KeyCount)
        {
            keysCurrent[static_cast<size_t>(event.key)] = 0;
        }
        break;
    case InputEvent::Type::MouseButtonDown:
        mouseButtonsCurrent |= buttonMask(event.button);
        break;
    case InputEvent::Type::MouseButtonUp:
        mouseButtonsCurrent &= ~buttonMask(event.button);
        break;
    case InputEvent::Type::MouseMove:
        mouseX = event.x;
        mouseY = event.y;
        mouseDeltaX = event.deltaX;
        mouseDeltaY = event.deltaY;
        break;
    case InputEvent::Type::MouseWheel:
        mouseWheel += event.wheel;
        break;
    case InputEvent::Type::TextInput:
        text += event.text;
        break;
    default:
        break;
    }
}

bool InputSystem::keyDown(KeyCode key) const
{
    const size_t index = static_cast<size_t>(key);
    return index < KeyCount && keysCurrent[index] != 0;
}

bool InputSystem::keyPressed(KeyCode key) const
{
    const size_t index = static_cast<size_t>(key);
    return index < KeyCount && keysCurrent[index] != 0 && keysPrevious[index] == 0;
}

bool InputSystem::keyReleased(KeyCode key) const
{
    const size_t index = static_cast<size_t>(key);
    return index < KeyCount && keysCurrent[index] == 0 && keysPrevious[index] != 0;
}

bool InputSystem::mouseButtonDown(MouseButton button) const
{
    return (mouseButtonsCurrent & buttonMask(button)) != 0;
}

bool InputSystem::mouseButtonPressed(MouseButton button) const
{
    const uint32_t mask = buttonMask(button);
    return (mouseButtonsCurrent & mask) != 0 && (mouseButtonsPrevious & mask) == 0;
}

bool InputSystem::mouseButtonReleased(MouseButton button) const
{
    const uint32_t mask = buttonMask(button);
    return (mouseButtonsCurrent & mask) == 0 && (mouseButtonsPrevious & mask) != 0;
}

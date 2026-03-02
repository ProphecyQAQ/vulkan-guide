#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <utility>

enum class KeyCode : uint16_t
{
    Unknown = 0,
    W,
    A,
    S,
    D,
    Space,
    Escape,
    Up,
    Down,
    Left,
    Right,
    Count
};

enum class MouseButton : uint8_t
{
    Left = 1,
    Middle = 2,
    Right = 3,
    X1 = 4,
    X2 = 5
};

struct InputEvent
{
    enum class Type
    {
        Quit,
        KeyDown,
        KeyUp,
        MouseButtonDown,
        MouseButtonUp,
        MouseMove,
        MouseWheel,
        TextInput
    };

    Type type = Type::Quit;
    KeyCode key = KeyCode::Unknown;
    bool repeat = false;
    MouseButton button = MouseButton::Left;
    int x = 0;
    int y = 0;
    int deltaX = 0;
    int deltaY = 0;
    int wheel = 0;
    std::string text;
};

class InputSystem
{
public:
    static InputSystem& get();

    void beginFrame();
    void processEvent(const InputEvent& event);

    bool quitRequested() const { return quit; }

    bool keyDown(KeyCode key) const;
    bool keyPressed(KeyCode key) const;
    bool keyReleased(KeyCode key) const;

    bool mouseButtonDown(MouseButton button) const;
    bool mouseButtonPressed(MouseButton button) const;
    bool mouseButtonReleased(MouseButton button) const;

    std::pair<int, int> mousePosition() const { return { mouseX, mouseY }; }
    std::pair<int, int> mouseDeltaPosition() const { return { mouseDeltaX, mouseDeltaY}; }
    int mouseWheelDelta() const { return mouseWheel; }
    const std::string& textInput() const { return text; }

private:
    InputSystem() = default;

    static constexpr size_t KeyCount = static_cast<size_t>(KeyCode::Count);

    std::array<uint8_t, KeyCount> keysCurrent{};
    std::array<uint8_t, KeyCount> keysPrevious{};

    uint32_t mouseButtonsCurrent = 0;
    uint32_t mouseButtonsPrevious = 0;
    int mouseX = 0;
    int mouseY = 0;
    int mouseDeltaX = 0;
    int mouseDeltaY = 0;
    int mouseWheel = 0;

    bool quit = false;
    std::string text;
};

# engine/input - Input System

## Overview
Unified input management for keyboard, mouse, and gamepad (XInput). Provides state tracking with pressed/down/up detection and analog input processing with dead zone support.

## Files
```
key.h              : Key and MouseButton enums (Windows virtual key codes)
keyboard.h/cpp     : Keyboard class (key state, hold time, modifiers)
mouse.h/cpp        : Mouse class (position, delta, buttons, wheel)
gamepad.h/cpp      : Gamepad class (XInput single controller)
gamepad_manager.h/cpp : GamepadManager (4 controllers)
input_manager.h/cpp   : InputManager singleton (unified access)
```

## Class Hierarchy
```
InputManager (Singleton)
  ├─ Keyboard (unique_ptr)
  ├─ Mouse (unique_ptr)
  └─ GamepadManager (unique_ptr)
        └─ Gamepad[4] (unique_ptr array)
```

## Class Details

### Key (enum class)
- Role: Windows virtual key codes
- Categories:
  - A-Z, 0-9 (0x41-0x5A, 0x30-0x39)
  - F1-F12 (0x70-0x7B)
  - Control: Escape, Enter, Tab, Backspace, Space, Delete, Insert, Home, End, PageUp/Down
  - Arrows: Left, Up, Right, Down
  - Modifiers: LeftShift, RightShift, LeftControl, RightControl, LeftAlt, RightAlt
  - Numpad: Numpad0-9, operators
  - Symbols: Semicolon, Plus, Comma, Minus, etc.
- Constant: `KeyCount = 256`

### MouseButton (enum class)
```cpp
enum class MouseButton {
    Left = 0, Right = 1, Middle = 2,
    X1 = 3, X2 = 4,  // Side buttons
    ButtonCount = 5
};
```

### GamepadButton (enum class)
- Role: XInput button masks
- Values: DPadUp/Down/Left/Right, Start, Back, LeftThumb, RightThumb, LeftShoulder, RightShoulder, A, B, X, Y

### Keyboard
- Role: Keyboard input state management
- State Detection:
  ```cpp
  bool IsKeyPressed(Key key);  // Currently held
  bool IsKeyDown(Key key);     // Just pressed this frame
  bool IsKeyUp(Key key);       // Just released this frame
  float GetKeyHoldTime(Key key);  // Seconds held
  ```
- Modifiers:
  ```cpp
  bool IsShiftPressed();    // Left or Right Shift
  bool IsControlPressed();  // Left or Right Ctrl
  bool IsAltPressed();      // Left or Right Alt
  ```
- Internal:
  ```cpp
  void Update(float deltaTime);  // Poll via GetAsyncKeyState
  void OnKeyDown(int virtualKey);  // WM_KEYDOWN handler
  void OnKeyUp(int virtualKey);    // WM_KEYUP handler
  ```
- State Storage: `std::array<KeyState, 256>` where KeyState = {pressed, down, up, holdTime}

### Mouse
- Role: Mouse input state management
- Position:
  ```cpp
  int GetX();      // Client area X
  int GetY();      // Client area Y
  int GetDeltaX(); // Frame delta X
  int GetDeltaY(); // Frame delta Y
  ```
- Wheel:
  ```cpp
  float GetWheelDelta();  // Positive = up, negative = down
  ```
- Buttons:
  ```cpp
  bool IsButtonPressed(MouseButton);
  bool IsButtonDown(MouseButton);
  bool IsButtonUp(MouseButton);
  ```
- Internal:
  ```cpp
  void Update(HWND hwnd = nullptr);  // GetCursorPos + GetAsyncKeyState
  void SetPosition(int x, int y);
  void OnButtonDown(MouseButton);
  void OnButtonUp(MouseButton);
  void OnWheel(float delta);  // WM_MOUSEWHEEL handler
  ```

### Gamepad
- Role: Single XInput controller
- Constructor: `Gamepad(DWORD userIndex)` (0-3)
- Connection:
  ```cpp
  bool IsConnected();
  ```
- Buttons:
  ```cpp
  bool IsButtonPressed(GamepadButton);
  bool IsButtonDown(GamepadButton);
  bool IsButtonUp(GamepadButton);
  ```
- Analog:
  ```cpp
  float GetLeftStickX();   // -1.0 to 1.0
  float GetLeftStickY();   // -1.0 to 1.0
  float GetRightStickX();  // -1.0 to 1.0
  float GetRightStickY();  // -1.0 to 1.0
  float GetLeftTrigger();  // 0.0 to 1.0
  float GetRightTrigger(); // 0.0 to 1.0
  ```
- Dead Zone:
  ```cpp
  void SetDeadZone(float threshold);  // 0.0 to 1.0
  float GetDeadZone();                // Default: 0.2 (20%)
  ```
- Dead Zone Processing: Remaps value from [deadZone, 1.0] to [0.0, 1.0]

### GamepadManager
- Role: Manage 4 XInput controllers
- Constant: `MaxGamepads = 4`
- Methods:
  ```cpp
  void Update();                    // Update all gamepads
  bool IsConnected(int index);      // 0-3
  Gamepad& GetGamepad(int index);   // Returns gamepad[0] if out of range
  int GetConnectedCount();
  ```

### InputManager
- Role: Singleton entry point for all input
- Lifecycle:
  ```cpp
  static bool Initialize();
  static void Uninit();
  static InputManager* GetInstance();
  ```
- Accessors:
  ```cpp
  Keyboard& GetKeyboard();
  Mouse& GetMouse();
  GamepadManager& GetGamepadManager();
  ```
- Update:
  ```cpp
  void Update(float deltaTime);  // Calls Keyboard/Mouse/Gamepad Update
  ```

## Data Flow
```
[Win32 Messages]                    [Per-Frame Polling]
WM_KEYDOWN/WM_KEYUP                 GetAsyncKeyState (Keyboard)
WM_LBUTTONDOWN/WM_LBUTTONUP         GetCursorPos (Mouse)
WM_MOUSEWHEEL                       XInputGetState (Gamepad)
        │                                    │
        ▼                                    ▼
Keyboard::OnKeyDown/OnKeyUp         Keyboard::Update(deltaTime)
Mouse::OnButtonDown/OnButtonUp      Mouse::Update(hwnd)
Mouse::OnWheel                      Gamepad::Update()
        │                                    │
        └────────────────┬───────────────────┘
                         ▼
              InputManager::Update(deltaTime)
                         │
                         ▼
              [User Code Query]
              keyboard.IsKeyDown(Key::Space)
              mouse.IsButtonPressed(MouseButton::Left)
              gamepad.GetLeftStickX()
```

## Input State Pattern
All input classes use the same state tracking:
```cpp
struct State {
    bool pressed;   // Currently held
    bool down;      // Transition: not pressed → pressed (1 frame)
    bool up;        // Transition: pressed → not pressed (1 frame)
};
```
- `pressed`: True while held
- `down`: True only on the frame the input starts
- `up`: True only on the frame the input ends

## Constants
| Constant | Value | Description |
|----------|-------|-------------|
| Key::KeyCount | 256 | Virtual key array size |
| MouseButton::ButtonCount | 5 | Mouse button count |
| GamepadManager::MaxGamepads | 4 | XInput max controllers |
| Default dead zone | 0.2 | 20% stick dead zone |
| Stick range | -32768 to 32767 | XInput raw value |
| Trigger range | 0 to 255 | XInput raw value |

## Caveats
1. **Dual input paths**: Both message handlers (OnKeyDown) and polling (Update) update state; polling overwrites message state
2. **Wheel delta reset**: `wheelDelta_` reset to 0 each frame in `Mouse::Update()`
3. **Gamepad disabled**: `GamepadManager` created but not updated in `InputManager::Update()` (commented out)
4. **Out-of-range gamepad**: `GetGamepad(invalid)` returns gamepad[0], not null
5. **No repeat detection**: Keyboard OnKeyDown ignores WM_KEYDOWN repeat flag (checks `!pressed`)
6. **Hold time accumulates**: Only during polling path, not message path

## Extension Points
| Extension | Location |
|-----------|----------|
| Input binding system | Create ActionMap class wrapping InputManager |
| Input recording | Add recording layer to InputManager::Update |
| Rumble/vibration | Add XInputSetState to Gamepad |
| Raw input | Add WM_INPUT handler for high-precision mouse |
| Touch input | Add Touch class to InputManager |
| Multiple keyboards | Use Raw Input API |

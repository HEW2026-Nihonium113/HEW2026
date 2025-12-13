# engine/platform - Application Framework

## Overview
Win32 application framework providing window management, main loop, time management, and rendering infrastructure. Entry point for game initialization and execution.

## Files
```
window.h/cpp       : Window class (Win32 window wrapper, RAII)
application.h/cpp  : Application singleton (main loop, time, subsystem init)
application.inl    : Application template implementation (Run/MainLoop)
renderer.h/cpp     : Renderer singleton (SwapChain, fixed-resolution buffers)
```

## Class Hierarchy
```
Application (Singleton)
  ├─ Window (unique_ptr)
  │    └─ HWND
  └─ Renderer::Get() (reference to singleton)
        ├─ SwapChain (unique_ptr)
        ├─ colorBuffer_ (TexturePtr, fixed resolution)
        └─ depthBuffer_ (TexturePtr, fixed resolution)
```

## Class Details

### WindowDesc
```cpp
struct WindowDesc {
    HINSTANCE hInstance;           // Application instance
    std::wstring title;            // Window title (default: L"mutra Application")
    uint32_t width = 1280;         // Client area width
    uint32_t height = 720;         // Client area height
    bool resizable = true;         // Allow resize
    uint32_t minWidth = 320;       // Minimum width (WM_GETMINMAXINFO)
    uint32_t minHeight = 240;      // Minimum height
};
```

### Window
- Role: Win32 window wrapper with RAII
- Constructor: `Window(const WindowDesc& desc)`
- Message Processing:
  ```cpp
  bool ProcessMessages();  // PeekMessage loop, returns false on WM_QUIT
  ```
- State Queries:
  ```cpp
  bool IsValid();          // hwnd_ != nullptr
  bool ShouldClose();      // WM_CLOSE received
  HWND GetHWND();
  uint32_t GetWidth();     // Client area
  uint32_t GetHeight();
  float GetAspectRatio();
  bool IsFocused();
  bool IsMinimized();
  ```
- Control:
  ```cpp
  void RequestClose();     // Sets shouldClose_ = true
  ```
- Callbacks:
  ```cpp
  void SetResizeCallback(ResizeCallback);  // (uint32_t w, uint32_t h)
  void SetFocusCallback(FocusCallback);    // (bool focused)
  ```
- Message Handling: WM_SIZE, WM_ACTIVATE, WM_SETFOCUS, WM_KILLFOCUS, WM_GETMINMAXINFO, WM_CLOSE, WM_DESTROY
- Internal: Static WndProc → instance HandleMessage via GWLP_USERDATA

### ApplicationDesc
```cpp
struct ApplicationDesc {
    HINSTANCE hInstance;           // Auto-detected if nullptr
    WindowDesc window;             // Window settings
    uint32_t renderWidth = 1280;   // Fixed render resolution
    uint32_t renderHeight = 720;
    bool enableDebugLayer = true;  // D3D11 debug layer
    VSyncMode vsync = VSyncMode::On;
    float maxDeltaTime = 0.25f;    // Cap for debugging (breakpoints)
};
```

### Application
- Role: Singleton application manager
- Lifecycle:
  ```cpp
  static Application& Get() noexcept;
  bool Initialize(const ApplicationDesc& desc);
  template<typename TGame> void Run(TGame& game);
  void Shutdown() noexcept;
  void Quit() noexcept;  // Request loop exit
  ```
- Time Management:
  ```cpp
  float GetDeltaTime();    // Seconds since last frame (capped)
  float GetTotalTime();    // Seconds since start
  float GetFPS();          // Updated every 1 second
  uint64_t GetFrameCount();
  ```
- Subsystem Access:
  ```cpp
  HINSTANCE GetHInstance();
  HWND GetHWND();
  Window* GetWindow();
  ```
- Game Loop (template TGame must have):
  ```cpp
  void Update();
  void Render();
  void EndFrame();
  ```

### Renderer
- Role: Singleton managing SwapChain and fixed-resolution render targets
- Lifecycle:
  ```cpp
  static Renderer& Get() noexcept;
  bool Initialize(HWND, windowW, windowH, renderW, renderH, vsync);
  void Shutdown() noexcept;
  bool IsValid() const;
  ```
- Drawing:
  ```cpp
  void Present();                      // SwapChain Present
  void Resize(uint32_t w, uint32_t h); // SwapChain only (buffers fixed)
  ```
- Resource Access:
  ```cpp
  Texture* GetColorBuffer();   // Fixed resolution render target
  Texture* GetDepthBuffer();   // Fixed resolution depth stencil
  Texture* GetBackBuffer();    // SwapChain back buffer
  SwapChain* GetSwapChain();
  uint32_t GetRenderWidth();
  uint32_t GetRenderHeight();
  ```

## Data Flow
```
[WinMain]
     │
     ▼
Application::Get().Initialize(desc)
     │
     ├─ Window creation
     ├─ GraphicsDevice::Initialize()
     ├─ GraphicsContext::Initialize()
     └─ Renderer::Initialize()
           ├─ SwapChain creation
           ├─ colorBuffer_ (fixed resolution)
           └─ depthBuffer_ (fixed resolution)
     │
     ▼
Application::Get().Run(game)
     │
     └─ MainLoop:
          ├─ Window::ProcessMessages()
          ├─ UpdateTime()
          ├─ ProcessInput() → InputManager::Update()
          ├─ game.Update()
          ├─ game.Render()
          ├─ Renderer::Present()
          └─ game.EndFrame()
     │
     ▼
Application::Get().Shutdown()
     │
     ├─ Renderer::Shutdown()
     ├─ GraphicsContext::Shutdown()
     ├─ GraphicsDevice::Shutdown()
     └─ Window destruction
```

## Initialization Order
```
1. Application::Initialize()
   ├─ Window (Win32 window creation)
   ├─ GraphicsDevice (ID3D11Device5)
   ├─ GraphicsContext (ID3D11DeviceContext4)
   └─ Renderer
        ├─ SwapChain (FLIP_DISCARD, double buffer)
        ├─ colorBuffer_ (R8G8B8A8_UNORM)
        └─ depthBuffer_ (D24_UNORM_S8_UINT)
```

## Shutdown Order
```
1. Application::Shutdown()
   ├─ ClearState() + Flush()
   ├─ Renderer::Shutdown()
   │    ├─ depthBuffer_.reset()
   │    ├─ colorBuffer_.reset()
   │    └─ swapChain_.reset()
   ├─ GraphicsContext::Shutdown()
   ├─ GraphicsDevice::Shutdown()
   └─ window_.reset()
```

## Main Loop Sequence
```cpp
while (!shouldQuit_) {
    if (!window_->ProcessMessages()) break;  // WM_QUIT
    if (window_->ShouldClose()) break;       // WM_CLOSE
    if (window_->IsMinimized()) { Sleep(10); continue; }

    UpdateTime();       // deltaTime, totalTime, FPS
    ProcessInput();     // InputManager::Update(deltaTime)
    game.Update();      // Game logic
    game.Render();      // Draw commands
    renderer.Present(); // SwapChain present
    game.EndFrame();    // Post-frame cleanup
    ++frameCount_;
}
```

## Constants
| Constant | Value | Description |
|----------|-------|-------------|
| Default window size | 1280x720 | Client area |
| Default render size | 1280x720 | Fixed resolution |
| Min window size | 320x240 | WM_GETMINMAXINFO |
| Max deltaTime | 0.25s | Cap for debug |
| FPS update interval | 1.0s | Average over 1 second |
| SwapChain buffers | 2 | Double buffering |
| SwapChain format | R8G8B8A8_UNORM | FLIP_DISCARD |

## Caveats
1. **Fixed render resolution**: colorBuffer/depthBuffer don't resize with window; only SwapChain resizes
2. **Minimized sleep**: Main loop sleeps 10ms when minimized to reduce CPU
3. **deltaTime cap**: maxDeltaTime prevents huge jumps after breakpoints
4. **FPS calculation**: Updated every 1 second, not instantaneous
5. **No sRGB back buffer**: FLIP_DISCARD doesn't support sRGB directly
6. **Quit vs Close**: `Quit()` sets flag, `RequestClose()` sets window flag
7. **Window class name**: Uses address-based unique name to support multiple windows

## Extension Points
| Extension | Location |
|-----------|----------|
| Fullscreen toggle | Add to Window or Renderer using SwapChain::SetFullscreen |
| Multiple windows | Create additional Window instances |
| Fixed timestep | Modify MainLoop with accumulator pattern |
| Frame limiting | Add Sleep/wait in MainLoop |
| Post-processing | Draw colorBuffer_ to backBuffer_ with shader |
| MSAA | Modify SwapChain creation with SampleDesc |

# dx11/core - Core D3D11 Infrastructure

## Overview
Core DirectX11 infrastructure: device creation, immediate context wrapper, swap chain management, and common headers. Entry point for all D3D11 operations.

## Files
```
gpu_common.h         : Central header (D3D11 includes, ComPtr, NonCopyable, helpers)
graphics_device.h/cpp: GraphicsDevice singleton (ID3D11Device5)
graphics_context.h/cpp: GraphicsContext singleton (ID3D11DeviceContext4)
swap_chain.h/cpp     : SwapChain (IDXGISwapChain3, back buffer, present)
```

## Class Hierarchy
```
GraphicsDevice (Singleton)
  └─ ID3D11Device5

GraphicsContext (Singleton)
  └─ ID3D11DeviceContext4

SwapChain (unique per window)
  ├─ IDXGISwapChain3
  ├─ TexturePtr (back buffer)
  └─ HANDLE (waitable object)
```

## Class Details

### gpu_common.h
- Role: Central include for all dx11 classes
- Provides:
  - `#include <d3d11_4.h>`, `<d3dcompiler.h>`, `<dxgi1_4.h>`
  - `ComPtr<T>` alias for `Microsoft::WRL::ComPtr<T>`
  - `NonCopyable`, `NonCopyableNonMovable` base classes
  - `GetCpuAccessFlags(D3D11_USAGE)` helper function

### GraphicsDevice
- Role: D3D11 device singleton
- Lifecycle:
  ```cpp
  static GraphicsDevice& Get() noexcept;
  bool Initialize(bool enableDebug = false);
  void Shutdown() noexcept;
  ```
- Accessor: `Device()` → `ID3D11Device5*`
- Global shortcut: `GetD3D11Device()` → `ID3D11Device5*`
- Debug: Reports live objects on shutdown in debug builds

### GraphicsContext
- Role: Immediate context singleton, command submission
- Lifecycle:
  ```cpp
  static GraphicsContext& Get() noexcept;
  bool Initialize() noexcept;
  void Shutdown() noexcept;
  ```
- Accessor: `GetContext()` → `ID3D11DeviceContext4*`

#### Command Categories
| Category | Methods |
|----------|---------|
| Draw | `Draw`, `DrawIndexed`, `DrawInstanced`, `DrawIndexedInstanced`, `*Indirect` |
| Compute | `Dispatch`, `DispatchIndirect` |
| Clear | `ClearRenderTarget`, `ClearDepthStencil`, `ClearUnorderedAccessView*` |
| RT/DS | `SetRenderTarget(s)`, `SetRenderTargetsAndUnorderedAccessViews` |
| Viewport | `SetViewport(s)`, `SetScissorRect(s)` |
| IA | `SetPrimitiveTopology`, `SetInputLayout`, `SetVertexBuffer(s)`, `SetIndexBuffer` |
| Buffers | `UpdateBuffer`, `MapBuffer`, `UnmapBuffer`, `UpdateConstantBuffer` |
| CB | `Set{VS/PS/GS/HS/DS/CS}ConstantBuffer` |
| SRV | `Set{VS/PS/GS/HS/DS/CS}ShaderResource(View)` (Texture or Buffer) |
| Sampler | `Set{VS/PS/GS/HS/DS/CS}Sampler` |
| State | `SetBlendState`, `SetDepthStencilState`, `SetRasterizerState` |
| Shader | `Set{Vertex/Pixel/Geometry/Hull/Domain/Compute}Shader` |
| UAV | `SetCSUnorderedAccessView`, `SetCSUnorderedAccessViewDirect` |
| Copy | `CopyResource`, `CopyStructureCount`, `UpdateSubresource` |
| Map | `Map`, `Unmap` |

### SwapChain
- Role: DXGI swap chain management
- Constructor: `SwapChain(HWND, DXGI_SWAP_CHAIN_DESC1&, fullscreenDesc*)`
- Methods:
  ```cpp
  bool Present(VSyncMode mode = VSyncMode::On);
  bool Resize(uint32_t width, uint32_t height);
  bool SetFullscreen(bool fullscreen);
  bool IsFullscreen() const;
  ```
- Accessors: `GetBackBuffer()`, `GetSwapChain()`, `IsValid()`
- Features:
  - sRGB RTV for back buffer (gamma correction)
  - Frame latency waitable object support
  - Alt+Enter disabled

### VSyncMode (enum)
| Value | Sync | Hz |
|-------|------|----|
| Off | No | Unlimited |
| On | Yes | 60 |
| Half | Yes | 30 |

## Initialization Order
```
1. GraphicsDevice::Get().Initialize(enableDebug)
   └─ D3D11CreateDevice → ID3D11Device5

2. GraphicsContext::Get().Initialize()
   └─ GetImmediateContext → ID3D11DeviceContext4

3. SwapChain(hwnd, desc)
   └─ CreateSwapChainForHwnd → IDXGISwapChain3
   └─ GetBuffer(0) → Back buffer Texture
```

## Shutdown Order
```
1. SwapChain destructor
   └─ backBuffer_.reset()
   └─ swapChain_.Reset()
   └─ CloseHandle(waitableObject_)

2. GraphicsContext::Get().Shutdown()
   └─ ClearState() → Flush() → Reset()

3. GraphicsDevice::Get().Shutdown()
   └─ ReportLiveDeviceObjects (debug)
   └─ device_.Reset()
```

## Data Flow
```
[User Code]
     │
     ▼
GraphicsContext::Set*()  ──▶  ID3D11DeviceContext4
GraphicsContext::Draw*()
     │
     ▼
SwapChain::Present() ──▶ IDXGISwapChain3::Present()
     │
     ▼
[Display]
```

## Caveats
1. **Initialize order matters**: Device → Context → SwapChain
2. **ClearState on shutdown**: Context clears all bindings before release
3. **sRGB back buffer**: RTV uses addSrgb() for gamma-correct output
4. **Dynamic buffer partial update**: Uses WRITE_DISCARD, loses previous data
5. **Null safety**: All context methods check for null before D3D11 calls
6. **Feature level**: Requires D3D_FEATURE_LEVEL_11_0 or 11_1

## Extension Points
| Extension | Location |
|-----------|----------|
| Deferred context | Add `CreateDeferredContext()` to GraphicsDevice |
| Multiple swap chains | Create additional SwapChain instances |
| MSAA | Set sampleCount/sampleQuality in DXGI_SWAP_CHAIN_DESC1 |
| HDR output | Use DXGI_FORMAT_R16G16B16A16_FLOAT for back buffer |
| Variable refresh | Add DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING support |

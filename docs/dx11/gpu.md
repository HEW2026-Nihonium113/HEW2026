# dx11/gpu - GPU Resource Management

## Overview
Unified GPU resource management (Buffer, Texture, Shader) for DirectX11. Uses `shared_ptr` ownership and descriptor-based factory pattern.

## Files
```
gpu.h           : Unified header (includes all resources)
gpu_resource.h  : Common definitions (DxException, AlignGpuSize, DX_CHECK)
intrusive_ptr.h : Intrusive ref-counting (RefCounted, IntrusivePtr) - unused
buffer.h/cpp    : Buffer (vertex/index/constant/structured)
texture.h/cpp   : Texture (1D/2D/3D/Cube/RenderTarget/DepthStencil)
shader.h/cpp    : Shader (VS/PS/GS/CS/HS/DS)
format.h/cpp    : DXGI_FORMAT utility
```

## Class Hierarchy
```
Buffer (shared_ptr)
  ├─ BufferDesc (24 bytes)
  ├─ ID3D11Buffer
  ├─ ShaderResourceView (optional)
  └─ UnorderedAccessView (optional)

Texture (shared_ptr)
  ├─ TextureDesc (48 bytes)
  ├─ ID3D11Resource (1D/2D/3D)
  ├─ SRV/RTV/DSV/UAV (per usage)
  └─ TextureDimension (enum)

Shader (shared_ptr)
  ├─ ID3D11DeviceChild (actual shader)
  └─ ID3DBlob (bytecode for input layout)

Format (value type utility)
  └─ DXGI_FORMAT conversion/info
```

## Class Details

### BufferDesc
- Size: 24 bytes (static_assert verified)
- Members: `size`, `stride`, `usage`, `bindFlags`, `cpuAccess`, `miscFlags`
- Factories:
  - `BufferDesc::Vertex(byteSize, dynamic)`
  - `BufferDesc::Index(byteSize, dynamic)`
  - `BufferDesc::Constant(byteSize)` - always DYNAMIC
  - `BufferDesc::Structured(elementSize, count, uav)`

### Buffer
- Role: GPU buffer creation/management
- Factories:
  ```cpp
  static BufferPtr Create(desc, initialData);
  static BufferPtr CreateVertex(byteSize, stride, dynamic, initialData);
  static BufferPtr CreateIndex(byteSize, dynamic, initialData);
  static BufferPtr CreateConstant(byteSize);
  static BufferPtr CreateStructured(elementSize, elementCount, withUav, initialData);
  ```
- Accessors: `Get()`, `Srv()`, `Uav()`, `Size()`, `Stride()`, `IsDynamic()`, `IsStructured()`

### TextureDesc
- Size: 48 bytes (static_assert verified)
- Members: `width`, `height`, `depth`, `mipLevels`, `arraySize`, `format`, `usage`, `bindFlags`, `cpuAccess`, `sampleCount`, `sampleQuality`, `dimension`
- Factories:
  - `TextureDesc::Tex1D/Tex2D/Tex3D(...)`
  - `TextureDesc::RenderTarget(w, h, fmt)` - SRV+RTV
  - `TextureDesc::DepthStencil(w, h, fmt)` - DSV
  - `TextureDesc::Uav(w, h, fmt)` - SRV+UAV
  - `TextureDesc::Cube(size, fmt)`

### Texture
- Role: GPU texture creation/management
- Factories:
  ```cpp
  static TexturePtr Create2D(w, h, format, initialData);
  static TexturePtr CreateRenderTarget(w, h, format);
  static TexturePtr CreateDepthStencil(w, h, format, withSrv);
  static TexturePtr CreateUav(w, h, format);
  static TexturePtr CreateCube(size, format);
  ```
- Depth SRV: When `withSrv=true`, uses Typeless format internally with separate view formats for DSV/SRV
- Destructor: Explicit release order (views → resource) with refcount logging

### Shader
- Role: GPU shader object creation/management
- Factories:
  ```cpp
  static ShaderPtr CreateVertexShader(bytecode);
  static ShaderPtr CreatePixelShader(bytecode);
  static ShaderPtr CreateGeometryShader(bytecode);
  static ShaderPtr CreateComputeShader(bytecode);
  static ShaderPtr CreateHullShader(bytecode);
  static ShaderPtr CreateDomainShader(bytecode);
  ```
- Cast: `AsVs()`, `AsPs()`, `AsGs()`, `AsCs()`, `AsHs()`, `AsDs()`
- Type check: `IsVertex()`, `IsPixel()`, ... (uses QueryInterface)
- Bytecode: `Bytecode()`, `BytecodeSize()`, `HasBytecode()`

### Format
- Role: DXGI_FORMAT conversion/info utility
- Conversion: `typeless()`, `toColor()`, `toDepth()`, `addSrgb()`, `removeSrgb()`
- Info: `bpp()` (bits per pixel), `isDepthStencil()`

### Enums
```cpp
enum class TextureDimension : uint32_t { Tex1D=0, Tex2D=1, Tex3D=2, Cube=3 };
enum class CubeFace : uint32_t { PositiveX=0, ..., NegativeZ=5, Count=6 };
```

## Data Flow
```
[BufferDesc/TextureDesc]
        │
        ▼
Buffer/Texture::Create*()
        │ GraphicsDevice → D3D11 resource
        ▼
[ID3D11Buffer / ID3D11Texture*]
        │ Auto view creation per bindFlags
        ▼
[SRV/RTV/DSV/UAV]
        │
        ▼
shared_ptr<Buffer/Texture>
```

## Constants
| Constant | Value | Description |
|----------|-------|-------------|
| `kGpuAlignment` | 16 | GPU memory alignment |
| `BufferDesc` size | 24 bytes | Descriptor size |
| `TextureDesc` size | 48 bytes | Descriptor size |
| Default format | `R8G8B8A8_UNORM` | Texture |
| Default depth | `D24_UNORM_S8_UINT` | Depth stencil |

## Caveats
1. **Buffer size auto-aligned**: `AlignGpuSize()` rounds up to 16-byte boundary
2. **Constant buffer always DYNAMIC**: `BufferDesc::Constant()` sets `D3D11_USAGE_DYNAMIC`
3. **Depth SRV format conversion**: `CreateDepthStencil(withSrv=true)` uses Typeless internally
4. **Shader retains bytecode**: Needed for vertex shader input layout creation
5. **Texture destructor logging**: Logs resource refcount for leak detection
6. **IntrusivePtr unused**: `mutra::RefCounted` defined but not currently used

## Extension Points
| Extension | Location |
|-----------|----------|
| New texture type | Add factory to `TextureDesc` + `Texture::Create*` |
| Streaming buffer | Add staging factory to `BufferDesc` |
| Mipmap generation | Add `mipLevels` support to `Texture::Create2D` |
| Use intrusive ptr | Inherit `RefCounted` + wrap with `IntrusivePtr` |
| New format support | Add cases to `Format` class switches |

# dx11/state - Pipeline State Objects

## Overview
Wrappers for D3D11 pipeline state objects (Blend, DepthStencil, Rasterizer, Sampler). Uses factory pattern with preset configurations for common use cases.

## Files
```
blend_state.h/cpp         : BlendState (alpha blending)
depth_stencil_state.h/cpp : DepthStencilState (depth/stencil test)
rasterizer_state.h/cpp    : RasterizerState (fill mode, culling)
sampler_state.h/cpp       : SamplerState (texture filtering)
```

## Class Hierarchy
```
BlendState (unique_ptr, NonCopyable)
  └─ ID3D11BlendState

DepthStencilState (unique_ptr, NonCopyable)
  └─ ID3D11DepthStencilState

RasterizerState (unique_ptr, NonCopyable)
  └─ ID3D11RasterizerState

SamplerState (unique_ptr, NonCopyable)
  └─ ID3D11SamplerState
```

## Class Details

### BlendState
- Role: Encapsulate alpha blending configuration
- Factories:
  ```cpp
  static unique_ptr<BlendState> Create(D3D11_BLEND_DESC&);
  static unique_ptr<BlendState> CreateOpaque();           // No blending
  static unique_ptr<BlendState> CreateAlphaBlend();       // SrcAlpha / InvSrcAlpha
  static unique_ptr<BlendState> CreateAdditive();         // SrcAlpha / One
  static unique_ptr<BlendState> CreateMultiply();         // Zero / SrcColor
  static unique_ptr<BlendState> CreatePremultipliedAlpha(); // One / InvSrcAlpha
  ```
- Accessor: `GetD3DBlendState()`, `IsValid()`

### DepthStencilState
- Role: Encapsulate depth/stencil test configuration
- Factories:
  ```cpp
  static unique_ptr<DepthStencilState> Create(D3D11_DEPTH_STENCIL_DESC&);
  static unique_ptr<DepthStencilState> CreateDefault();   // DepthTest=ON, Write=ON, Less
  static unique_ptr<DepthStencilState> CreateReadOnly();  // DepthTest=ON, Write=OFF
  static unique_ptr<DepthStencilState> CreateDisabled();  // DepthTest=OFF
  static unique_ptr<DepthStencilState> CreateReversed();  // DepthFunc=Greater (reverse-Z)
  static unique_ptr<DepthStencilState> CreateLessEqual(); // DepthFunc=LessEqual
  ```
- Accessor: `GetD3DDepthStencilState()`, `IsValid()`

### RasterizerState
- Role: Encapsulate rasterization configuration
- Factories:
  ```cpp
  static unique_ptr<RasterizerState> Create(D3D11_RASTERIZER_DESC&);
  static unique_ptr<RasterizerState> CreateDefault();     // Solid, BackCull
  static unique_ptr<RasterizerState> CreateWireframe();   // Wireframe, BackCull
  static unique_ptr<RasterizerState> CreateNoCull();      // Solid, NoCull
  static unique_ptr<RasterizerState> CreateFrontCull();   // Solid, FrontCull
  static unique_ptr<RasterizerState> CreateShadowMap(depthBias, slopeScale); // Depth bias
  ```
- Accessor: `GetD3DRasterizerState()`, `IsValid()`

### SamplerState
- Role: Encapsulate texture sampling configuration
- Factories:
  ```cpp
  static unique_ptr<SamplerState> Create(D3D11_SAMPLER_DESC&);
  static unique_ptr<SamplerState> CreateDefault();        // Linear, Wrap
  static unique_ptr<SamplerState> CreatePoint();          // Point, Wrap
  static unique_ptr<SamplerState> CreateAnisotropic(maxAniso=16); // Anisotropic, Wrap
  static unique_ptr<SamplerState> CreateComparison();     // PCF shadow sampling
  ```
- Accessor: `GetD3DSamplerState()`, `IsValid()`

## Data Flow
```
[D3D11_*_DESC or Preset]
        │
        ▼
*State::Create*()
        │ GetD3D11Device() → CreateBlendState/etc.
        ▼
unique_ptr<*State>
        │
        ▼
context->OMSetBlendState() / RSSetState() / PSSetSamplers() / OMSetDepthStencilState()
```

## Preset Configurations

### BlendState Presets
| Preset | SrcBlend | DestBlend | Use Case |
|--------|----------|-----------|----------|
| Opaque | ONE | ZERO | Solid objects |
| AlphaBlend | SRC_ALPHA | INV_SRC_ALPHA | Transparent sprites |
| Additive | SRC_ALPHA | ONE | Particles, glow |
| Multiply | ZERO | SRC_COLOR | Shadow overlay |
| PremultipliedAlpha | ONE | INV_SRC_ALPHA | Pre-multiplied textures |

### DepthStencilState Presets
| Preset | DepthEnable | DepthWrite | DepthFunc | Use Case |
|--------|-------------|------------|-----------|----------|
| Default | ON | ON | Less | Standard 3D |
| ReadOnly | ON | OFF | Less | Transparent objects |
| Disabled | OFF | OFF | - | 2D overlay |
| Reversed | ON | ON | Greater | Reverse-Z buffer |
| LessEqual | ON | ON | LessEqual | Coplanar geometry |

### RasterizerState Presets
| Preset | FillMode | CullMode | DepthBias | Use Case |
|--------|----------|----------|-----------|----------|
| Default | Solid | Back | 0 | Standard |
| Wireframe | Wireframe | Back | 0 | Debug |
| NoCull | Solid | None | 0 | Double-sided |
| FrontCull | Solid | Front | 0 | Inside render |
| ShadowMap | Solid | Back | 100000 | Shadow maps |

### SamplerState Presets
| Preset | Filter | AddressMode | Use Case |
|--------|--------|-------------|----------|
| Default | Linear | Wrap | Standard textures |
| Point | Point | Wrap | Pixel art, UI |
| Anisotropic | Anisotropic(16) | Wrap | High-quality textures |
| Comparison | CompareLinear | Border(white) | PCF shadow sampling |

## Caveats
1. **Stencil not configured**: All depth stencil presets have StencilEnable=FALSE
2. **No caching**: Each Create*() call creates new D3D11 state object
3. **Anisotropy clamped**: CreateAnisotropic() clamps to 1-16 range
4. **Shadow sampler border**: CreateComparison() uses white border color (1,1,1,1)
5. **FrontCounterClockwise=FALSE**: All rasterizer states use clockwise front face

## Extension Points
| Extension | Location |
|-----------|----------|
| New blend preset | Add factory to `BlendState::Create*()` |
| Stencil operations | Add factory with StencilEnable=TRUE |
| Scissor test | Add factory with ScissorEnable=TRUE |
| Custom address mode | Add factory with Clamp/Mirror/Border |
| State caching | Create manager class with hash-based lookup |

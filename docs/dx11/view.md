# dx11/view - Resource View Wrappers

## Overview
Wrappers for D3D11 resource views (SRV, RTV, DSV, UAV). Provides factory methods for creating views from various resource types with optional descriptor override.

## Files
```
shader_resource_view.h/cpp   : ShaderResourceView (SRV) - shader read access
render_target_view.h/cpp     : RenderTargetView (RTV) - render output
depth_stencil_view.h/cpp     : DepthStencilView (DSV) - depth/stencil output
unordered_access_view.h/cpp  : UnorderedAccessView (UAV) - compute read/write
```

## Class Hierarchy
```
ShaderResourceView (unique_ptr, NonCopyable, Movable)
  └─ ID3D11ShaderResourceView

RenderTargetView (unique_ptr, NonCopyable, Movable)
  └─ ID3D11RenderTargetView

DepthStencilView (unique_ptr, NonCopyable, Movable)
  └─ ID3D11DepthStencilView

UnorderedAccessView (unique_ptr, NonCopyable, Movable)
  └─ ID3D11UnorderedAccessView
```

## Common Interface Pattern

All view classes share identical interface:

### Factories
```cpp
// From specific resource type (desc=nullptr uses default)
static unique_ptr<*View> CreateFromBuffer(ID3D11Buffer*, desc*);
static unique_ptr<*View> CreateFromTexture1D(ID3D11Texture1D*, desc*);
static unique_ptr<*View> CreateFromTexture2D(ID3D11Texture2D*, desc*);
static unique_ptr<*View> CreateFromTexture3D(ID3D11Texture3D*, desc*);

// From generic resource (desc required)
static unique_ptr<*View> Create(ID3D11Resource*, desc&);

// Wrap existing D3D11 view
static unique_ptr<*View> FromD3DView(ComPtr<ID3D11*View>);
```

### Accessors
```cpp
ID3D11*View* Get() const noexcept;
ID3D11*View* const* GetAddressOf() const noexcept;
bool IsValid() const noexcept;
ComPtr<ID3D11*View> Detach() noexcept;  // Transfer ownership
D3D11_*_VIEW_DESC GetDesc() const noexcept;
```

## Class Details

### ShaderResourceView (SRV)
- Role: Read-only shader access to resources
- Pipeline stages: VS, PS, GS, HS, DS, CS (all shader stages)
- Use cases: Textures, structured buffers, constant buffers

### RenderTargetView (RTV)
- Role: Write output destination for pixel shader
- Pipeline stages: Output Merger (OM)
- Use cases: Back buffer, off-screen render targets

### DepthStencilView (DSV)
- Role: Depth/stencil buffer access
- Pipeline stages: Output Merger (OM)
- Use cases: Depth testing, shadow maps
- Note: No CreateFromBuffer() (depth requires texture)

### UnorderedAccessView (UAV)
- Role: Random read/write access for compute shaders
- Pipeline stages: CS, PS (D3D11.1+)
- Use cases: Compute output, atomic operations

## Data Flow
```
[ID3D11Resource (Buffer/Texture)]
              │
              ▼
*View::CreateFrom*() or Create()
              │ GetD3D11Device() → Create*View()
              ▼
unique_ptr<*View>
              │
              ├──▶ context->PSSetShaderResources(srv)
              ├──▶ context->OMSetRenderTargets(rtv, dsv)
              └──▶ context->CSSetUnorderedAccessViews(uav)
```

## View Type Compatibility

| Resource Type | SRV | RTV | DSV | UAV |
|---------------|-----|-----|-----|-----|
| Buffer        | Yes | Yes | No  | Yes |
| Texture1D     | Yes | Yes | Yes | Yes |
| Texture2D     | Yes | Yes | Yes | Yes |
| Texture3D     | Yes | Yes | No  | Yes |

## Caveats
1. **Null descriptor = auto-detect**: Passing nullptr for desc uses D3D11 default based on resource format
2. **DSV no buffer support**: DepthStencilView has no CreateFromBuffer() (textures only)
3. **Ownership transfer**: Detach() moves ComPtr out, invalidating the view wrapper
4. **GetDesc() on invalid**: Returns zero-initialized struct if view is null
5. **Format compatibility**: View format must be compatible with resource format (typeless allows flexibility)

## Extension Points
| Extension | Location |
|-----------|----------|
| Cube map SRV | Use SRV_DIMENSION_TEXTURECUBE in desc |
| Array slice RTV | Specify FirstArraySlice/ArraySize in desc |
| Read-only DSV | Use D3D11_DSV_READ_ONLY_DEPTH flag |
| Append/Consume UAV | Use D3D11_BUFFER_UAV_FLAG_APPEND |
| View caching | Create manager with resource→view map |

# dx11/compile - Shader Compilation System

## Overview
Subsystem for HLSL shader compilation, caching, and reflection. Wraps D3DCompile API with swappable interface design.

## Files
```
shader_type.h          : ShaderType enum + helper functions
shader_types_fwd.h     : Forward declarations, ShaderDefine, ShaderCacheStats
shader_compiler.h/cpp  : IShaderCompiler, D3DShaderCompiler
shader_reflection.h/cpp: ShaderReflection (excluded from build)
```

Note: shader_cache.h/cpp moved to engine/shader/

## Class Hierarchy
```
IShaderCompiler (interface)
  └─ D3DShaderCompiler

ShaderReflection - standalone utility (excluded from build)
```

Note: IShaderCache and IShaderResourceCache moved to engine/shader/

## Class Details

### ShaderType (enum class)
- Values: `Vertex`, `Pixel`, `Geometry`, `Hull`, `Domain`, `Compute`, `Count`
- Helpers:
  - `GetShaderProfile(type)` → `"vs_5_0"` etc.
  - `GetShaderTypeName(type)` → `"Vertex"` etc.
  - `GetShaderEntryPoint(type)` → `"VSMain"` etc.
  - `IsGraphicsShader(type)` → true except Compute

### D3DShaderCompiler
- Role: Compile HLSL source to bytecode
- Method:
  ```cpp
  ShaderCompileResult compile(
      const std::vector<char>& source,
      const std::string& sourceName,
      const std::string& profile,
      const std::string& entryPoint,
      const std::vector<ShaderDefine>& defines = {}
  ) noexcept;
  ```
- Returns: `ShaderCompileResult { success, bytecode, errorMessage, warningMessage }`
- Compile flags:
  - Debug: `D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION`
  - Release: `D3DCOMPILE_OPTIMIZATION_LEVEL3`
  - Common: `D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR`

### ShaderReflection (excluded from build)
- Role: Extract parameter info from bytecode
- Factory: `ShaderReflection::Create(ID3DBlob*)` → `unique_ptr`
- Getters: `GetConstantBuffers()`, `GetTextures()`, `GetSamplers()`, `GetInputElements()`
- Utility: `BuildParameterMap()` → `ShaderParameterMap`

## Data Flow
```
[HLSL Source]
     │
     ▼
D3DShaderCompiler::compile()
     │  D3DCompile() → ShaderCompileResult
     ▼
[ID3DBlob bytecode]
     │
     └──▶ ShaderReflection::Create(blob)  (if enabled)
              │  D3DReflect() → Parse()
              ▼
         BuildParameterMap()
```

## Constants
| Constant | Value | Description |
|----------|-------|-------------|
| Shader Model | 5.0 | vs_5_0, ps_5_0, etc. |
| Entry points | VSMain, PSMain, etc. | Fixed per ShaderType |
| Matrix packing | Column-major | DirectXMath compatible |

## Caveats
1. **ShaderReflection excluded**: `shader_reflection.cpp` removed in premake5.lua. Depends on unimplemented `shader_parameter.h`.
2. **D3D_COMPILE_STANDARD_FILE_INCLUDE**: `#include` resolved relative to file path.
3. **Cache moved**: IShaderCache/IShaderResourceCache now in engine/shader/.

## Extension Points
| Extension | Location |
|-----------|----------|
| New shader type | Add to `ShaderType` enum + update helpers |
| Compiler options | Modify `D3DShaderCompiler::getCompileFlags()` |
| Enable reflection | Remove from premake exclusion + implement `shader_parameter.h` |

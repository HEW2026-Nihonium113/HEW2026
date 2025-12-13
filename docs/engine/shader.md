# engine/shader - Shader Management System

## Overview
High-level shader management with compilation, caching, and program binding. Wraps dx11/compile with file system integration and provides GlobalShader pattern for statically-defined shaders.

## Files
```
shader_manager.h/cpp   : ShaderManager singleton (load, compile, cache)
shader_program.h/cpp   : ShaderProgram (VS/PS/GS/HS/DS bundle + InputLayout)
shader_cache.h/cpp     : IShaderCache, IShaderResourceCache + implementations
global_shader.h        : GlobalShader base class + macros for static shader definitions
```

## Class Hierarchy
```
ShaderManager (Singleton)
  ├─ IReadableFileSystem* (file access)
  ├─ IShaderCompiler* (D3DCompile)
  ├─ IShaderCache* (bytecode cache, optional)
  ├─ IShaderResourceCache* (Shader object cache)
  └─ globalShaders_ (type_index → GlobalShader)

ShaderProgram
  ├─ ShaderPtr vs_, ps_, gs_, hs_, ds_
  └─ ID3D11InputLayout (cached)

GlobalShader (abstract base)
  └─ ShaderPtr shader_
```

## Class Details

### ShaderManager
- Role: Singleton for shader loading, compilation, and caching
- Lifecycle:
  ```cpp
  static ShaderManager& Get() noexcept;
  void Initialize(IReadableFileSystem*, IShaderCompiler*,
                  IShaderCache* = nullptr, IShaderResourceCache* = nullptr);
  void Shutdown();
  bool IsInitialized() const;
  ```
- Unified Load API:
  ```cpp
  ShaderPtr LoadShader(path, ShaderType, defines = {});
  ```
- Individual Load API:
  ```cpp
  ShaderPtr LoadVertexShader(path, defines = {});
  ShaderPtr LoadPixelShader(path, defines = {});
  ShaderPtr LoadGeometryShader(path, defines = {});
  ShaderPtr LoadHullShader(path, defines = {});
  ShaderPtr LoadDomainShader(path, defines = {});
  ShaderPtr LoadComputeShader(path, defines = {});
  ```
- Program Creation:
  ```cpp
  unique_ptr<ShaderProgram> CreateProgram(vsPath, psPath);
  unique_ptr<ShaderProgram> CreateProgram(vsPath, psPath, gsPath);
  unique_ptr<ShaderProgram> CreateProgram(vs, ps, gs = nullptr, hs = nullptr, ds = nullptr);
  ```
- GlobalShader:
  ```cpp
  template<typename T> T* GetGlobalShader();  // Lazy load + cache
  ```
- InputLayout:
  ```cpp
  ComPtr<ID3D11InputLayout> CreateInputLayout(Shader* vs, elements, numElements);
  ```
- Bytecode:
  ```cpp
  ComPtr<ID3DBlob> CompileBytecode(path, type, defines = {});
  ```
- Cache Management:
  ```cpp
  void ClearCache();           // All caches
  void ClearBytecodeCache();
  void ClearResourceCache();
  void ClearGlobalShaderCache();
  ShaderCacheStats GetCacheStats() const;
  ```

### ShaderProgram
- Role: Bundle VS/PS/GS/HS/DS with InputLayout caching
- Factories:
  ```cpp
  static unique_ptr<ShaderProgram> Create(ShaderPtr vs, ShaderPtr ps);
  static unique_ptr<ShaderProgram> Create(ShaderPtr vs, ShaderPtr ps, ShaderPtr gs,
                                          ShaderPtr hs = nullptr, ShaderPtr ds = nullptr);
  ```
- Pipeline Operations:
  ```cpp
  void Bind() const;    // Set all shaders + InputLayout to context
  void Unbind() const;  // Set all to nullptr
  ```
- InputLayout:
  ```cpp
  ID3D11InputLayout* GetOrCreateInputLayout(elements, numElements);  // Cached by hash
  ID3D11InputLayout* GetInputLayout() const;
  ```
- Accessors:
  ```cpp
  Shader* GetVertexShader() const;
  Shader* GetPixelShader() const;
  Shader* GetGeometryShader() const;
  Shader* GetHullShader() const;
  Shader* GetDomainShader() const;
  bool IsValid() const;  // vs_ && ps_
  ```

### GlobalShader
- Role: Base class for statically-defined shader types
- Virtual Methods (implement in derived):
  ```cpp
  virtual const char* GetSourcePath() const = 0;
  virtual ShaderType GetShaderType() const = 0;
  virtual std::vector<ShaderDefine> GetDefines() const { return {}; }
  virtual const char* GetEntryPoint() const { return nullptr; }
  ```
- Accessors:
  ```cpp
  Shader* GetShader() const;
  bool IsValid() const;
  const void* GetBytecode() const;
  size_t GetBytecodeSize() const;
  ```
- Usage Pattern:
  ```cpp
  // Define shader class
  class MyVertexShader : public GlobalShader {
  public:
      const char* GetSourcePath() const override { return "shaders:/my_vs.hlsl"; }
      ShaderType GetShaderType() const override { return ShaderType::Vertex; }
  };

  // Or use macro
  DECLARE_GLOBAL_SHADER(MyVertexShader, ShaderType::Vertex, "shaders:/my_vs.hlsl")

  // Get (lazy load)
  auto* shader = ShaderManager::Get().GetGlobalShader<MyVertexShader>();
  ```

### GlobalShaderTypeInfo
- Role: Helper for type_index operations
- Methods:
  ```cpp
  template<typename T> static std::type_index GetTypeIndex();
  template<typename T> static unique_ptr<GlobalShader> CreateInstance();
  ```

## Data Flow
```
[User Code]
     │ LoadShader("shaders:/vs.hlsl", ShaderType::Vertex)
     ▼
ShaderManager::LoadShader()
     │
     ├─ ComputeCacheKey() → FNV-1a hash
     │
     ├─ resourceCache_->Get(key) → Cache hit? Return
     │
     └─ CompileBytecode()
          │
          ├─ bytecodeCache_->find(key) → Cache hit? Return
          │
          ├─ fileSystem_->readAsChars(path)
          │
          ├─ compiler_->compile(source, profile, entryPoint, defines)
          │
          └─ bytecodeCache_->store(key, blob)
     │
     ▼
CreateShaderFromBytecode()
     │ device->CreateVertexShader() etc.
     ▼
resourceCache_->Put(key, shader)
     │
     ▼
ShaderPtr
```

## Cache Key Calculation
```cpp
uint64_t hash = Fnv1a(path);
hash = Fnv1a(profile, hash);      // "vs_5_0", "ps_5_0", etc.
for (auto& define : defines) {
    hash = Fnv1a(define.name, hash);
    hash = Fnv1a(define.value, hash);
}
```

## InputLayout Caching
ShaderProgram caches InputLayout by hashing:
- SemanticName, SemanticIndex, Format
- InputSlot, AlignedByteOffset
- InputSlotClass, InstanceDataStepRate

Only recreates if hash differs from cached.

## Macros
| Macro | Usage |
|-------|-------|
| `DECLARE_GLOBAL_SHADER(Class, Type, Path)` | Define complete GlobalShader class |
| `IMPLEMENT_GLOBAL_SHADER(Class, Type, Path)` | Implement inside existing class |

## Constants
| Constant | Value | Description |
|----------|-------|-------------|
| Entry points | VSMain, PSMain, etc. | From GetShaderEntryPoint() |
| Profiles | vs_5_0, ps_5_0, etc. | Shader Model 5.0 |

## Caveats
1. **Bytecode kept for VS only**: CreateShaderFromBytecode keeps ID3DBlob only for Vertex shaders (for InputLayout)
2. **Cache key includes defines**: Different define sets = different cache entries
3. **GlobalShader lazy load**: First GetGlobalShader<T>() triggers compile
4. **ShaderProgram requires VS+PS**: Create() returns nullptr if either is missing
5. **No automatic reload**: Cache must be cleared manually for hot reload
6. **IShaderResourceCache optional**: Creates internal ShaderResourceCache if not provided

## Extension Points
| Extension | Location |
|-----------|----------|
| Hot reload | Add file watcher + ClearCache on change |
| Precompiled shaders | Load bytecode directly, skip compile |
| Shader permutations | Use defines to generate variants |
| Async compilation | Add CompileBytecodeAsync() |
| Shader reflection | Enable dx11/compile/shader_reflection |

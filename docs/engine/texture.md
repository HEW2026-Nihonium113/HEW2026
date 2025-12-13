# engine/texture - Texture Management System

## Overview
Texture loading, decoding, and caching. Supports WIC formats (PNG, JPG, BMP) and DDS (BC compression, cubemaps, mipmaps). Uses weak reference caching by default.

## Files
```
texture_manager.h/cpp : TextureManager singleton (load, create, cache)
texture_loader.h/cpp  : ITextureLoader interface, WICTextureLoader, DDSTextureLoader
texture_cache.h/cpp   : ITextureCache interface, LRUTextureCache, WeakTextureCache
```

## Class Hierarchy
```
TextureManager (Singleton)
  ├─ IReadableFileSystem* (file access)
  ├─ WICTextureLoader (unique_ptr)
  ├─ DDSTextureLoader (unique_ptr)
  └─ WeakTextureCache (unique_ptr, default)

ITextureLoader (interface)
  ├─ WICTextureLoader (PNG, JPG, BMP, TIFF, GIF)
  └─ DDSTextureLoader (DDS via DirectXTex)

ITextureCache (interface)
  ├─ LRUTextureCache (strong ref, memory limit)
  └─ WeakTextureCache (weak ref, auto cleanup)
```

## Class Details

### TextureCacheStats
```cpp
struct TextureCacheStats {
    size_t textureCount;       // Cached texture count
    size_t hitCount;           // Cache hits
    size_t missCount;          // Cache misses
    size_t totalMemoryBytes;   // Total GPU memory
    double HitRate() const;    // hitCount / (hitCount + missCount)
};
```

### TextureData
- Role: Intermediate data from loader to D3D11 creation
- Members:
  ```cpp
  std::vector<uint8_t> pixels;           // Pixel data
  uint32_t width, height;
  uint32_t mipLevels, arraySize;
  DXGI_FORMAT format;
  bool isCubemap;
  std::vector<D3D11_SUBRESOURCE_DATA> subresources;  // Per mip/array
  ```

### TextureManager
- Role: Singleton for texture loading and creation
- Lifecycle:
  ```cpp
  static TextureManager& Get() noexcept;
  void Initialize(IReadableFileSystem* fileSystem);
  void Shutdown();
  bool IsInitialized() const;
  ```
- Loading:
  ```cpp
  TexturePtr LoadTexture2D(path, sRGB = true, generateMips = false);
  TexturePtr LoadTextureCube(path, sRGB = true, generateMips = true);
  ```
- Creation (no file):
  ```cpp
  TexturePtr Create2D(width, height, format, bindFlags, initialData, rowPitch);
  TexturePtr CreateRenderTarget(width, height, format);
  TexturePtr CreateDepthStencil(width, height, format);
  ```
- Cache:
  ```cpp
  void ClearCache();
  TextureCacheStats GetCacheStats() const;
  ```

### ITextureLoader
- Role: Decode file data to TextureData
- Methods:
  ```cpp
  virtual bool Load(const void* data, size_t size, TextureData& outData) = 0;
  virtual bool SupportsExtension(const char* ext) const = 0;
  ```

### WICTextureLoader
- Role: Windows Imaging Component loader
- Supported: `.png`, `.jpg`, `.jpeg`, `.bmp`, `.tiff`, `.gif`
- Output format: `DXGI_FORMAT_R8G8B8A8_UNORM`
- Thread-safe: Yes (thread_local COM initialization + factory)
- Implementation: IWICImagingFactory → Decoder → FormatConverter (32bppRGBA)

### DDSTextureLoader
- Role: DirectXTex DDS loader
- Supported: `.dds`
- Features: BC compression, cubemaps, mipmaps preserved
- Implementation: DirectX::LoadFromDDSMemory → ScratchImage → subresources

### ITextureCache
- Role: Texture caching interface
- Methods:
  ```cpp
  virtual TexturePtr Get(uint64_t key) = 0;
  virtual void Put(uint64_t key, TexturePtr texture) = 0;
  virtual void Clear() = 0;
  virtual size_t Count() const = 0;
  virtual size_t MemoryUsage() const = 0;
  ```

### LRUTextureCache
- Role: Strong reference cache with memory limit
- Constructor: `LRUTextureCache(maxMemoryBytes = 256MB)`
- Behavior:
  - Evicts oldest entries when exceeding memory limit
  - Get() moves entry to front (most recent)
  - Strong reference keeps texture alive
- Methods:
  ```cpp
  void SetMaxMemory(size_t bytes);
  size_t GetMaxMemory() const;
  void PurgeExpired();  // No-op (strong refs never expire)
  void Evict();         // Force eviction to memory limit
  ```
- Data structure: `std::list<CacheEntry>` (LRU order) + `unordered_map<key, iterator>`

### WeakTextureCache
- Role: Weak reference cache (no memory ownership)
- Behavior:
  - Does not extend texture lifetime (weak_ptr)
  - Get() returns nullptr if texture was destroyed
  - Auto-removes expired entries on Get()
- Methods:
  ```cpp
  size_t PurgeExpired();    // Remove all expired entries
  size_t ValidCount() const; // Count non-expired entries
  ```
- Default cache: TextureManager uses WeakTextureCache by default

## Data Flow
```
[User Code]
     │ LoadTexture2D("textures:/sprite.png", sRGB=true)
     ▼
TextureManager::LoadTexture2D()
     │
     ├─ ComputeCacheKey(path, sRGB, generateMips)
     │
     ├─ cache_->Get(key) → Cache hit? Return
     │
     ├─ fileSystem_->read(path)
     │
     ├─ GetLoaderForExtension(".png") → WICTextureLoader
     │
     └─ loader->Load(bytes, size, TextureData)
          │
          ▼
     CreateTextureWithViews()
          │ device->CreateTexture2D()
          │ device->CreateShaderResourceView() etc.
          ▼
     cache_->Put(key, texture)
          │
          ▼
     TexturePtr
```

## Cache Key Calculation
```cpp
uint64_t hash = Fnv1a(path);
uint8_t flags = (sRGB ? 1 : 0) | (generateMips ? 2 : 0);
hash = Fnv1a(&flags, sizeof(flags), hash);
```

## Format Handling
| Scenario | Format Conversion |
|----------|-------------------|
| sRGB=true | `Format(f).addSrgb()` |
| sRGB=false | `Format(f).removeSrgb()` |
| DepthStencil D24_S8 | Texture: R24G8_TYPELESS, DSV: D24_S8, SRV: R24_UNORM_X8_TYPELESS |
| DepthStencil D32 | Texture: R32_TYPELESS, DSV: D32_FLOAT, SRV: R32_FLOAT |

## Constants
| Constant | Value | Description |
|----------|-------|-------------|
| LRU default max | 256 MB | Default memory limit |
| WIC output | R8G8B8A8_UNORM | Always converts to RGBA32 |
| Cubemap array size | 6 | Fixed for cubemaps |

## Caveats
1. **generateMips reference count**: Using `D3D11_RESOURCE_MISC_GENERATE_MIPS` causes reference count issues at shutdown (known D3D11 issue). Default is `generateMips=false`.
2. **Cubemap runtime mips**: Runtime mipmap generation for cubemaps not supported.
3. **WeakTextureCache default**: TextureManager uses WeakTextureCache, not LRU. Textures are freed when no external references remain.
4. **WIC thread-local**: COM and IWICImagingFactory are thread_local. Each thread initializes separately.
5. **DDS cubemap square**: Cubemap width and height forced equal (height = width).
6. **Cache key includes flags**: Same path with different sRGB/generateMips = different cache entries.

## Extension Points
| Extension | Location |
|-----------|----------|
| New format loader | Implement ITextureLoader |
| Async loading | Add LoadTexture2DAsync() with std::future |
| Texture streaming | Add virtual texturing layer |
| Compression on load | Add BC compression in loader |
| Hot reload | Add file watcher + ClearCache |

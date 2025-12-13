# engine/fs - Mount-Based File System Abstraction

## Overview
Virtual file system with mount points, inspired by Nintendo Switch nn::fs. Abstracts file access through named mount points, enabling transparent switching between host file system and in-memory storage.

## Files
```
file_error.h/cpp          : FileError struct with error codes and messages
file_system_types.h       : Result types (FileReadResult, DirectoryEntry, AsyncReadHandle)
file_system.h             : Interface hierarchy (IFileSystem, IReadableFileSystem, IWritableFileSystem, IFileHandle)
file_system_manager.h/cpp : FileSystemManager singleton (mount management)
file_system_functions.h   : Convenience free functions (MountHostFileSystem, ReadFile, etc.)
host_file_system.h/cpp    : HostFileSystem (Windows file system implementation)
memory_file_system.h/cpp  : MemoryFileSystem (in-memory, for tests/embedded resources)
file_watcher.h/cpp        : FileWatcher (directory change monitoring for hot reload)
path_utility.h            : PathUtility (path manipulation, UTF conversion)
```

## Class Hierarchy
```
IFileSystem (base interface)
  └─ IReadableFileSystem
       └─ IWritableFileSystem
             └─ HostFileSystem (Windows implementation)
       └─ MemoryFileSystem (read-only in-memory)

IFileHandle (interface)
  ├─ HostFileHandle (HANDLE wrapper)
  └─ MemoryFileHandle (byte vector wrapper)

FileSystemManager (Singleton)
  └─ MountPoint[] (name → IReadableFileSystem)

FileWatcher (Pimpl)
  └─ Impl (ReadDirectoryChangesW thread)

PathUtility (static methods)
```

## Class Details

### FileError
- Role: Error information container
- Members:
  - `code`: Abstract error code (NotFound, AccessDenied, InvalidPath, InvalidMount, etc.)
  - `nativeError`: OS-specific error (GetLastError() result)
  - `context`: Path or additional context
- Factory: `FileError::make(code, nativeError, context)`
- Method: `message()` → human-readable error string

### FileReadResult / FileOperationResult
```cpp
struct FileReadResult {
    bool success;
    FileError error;
    std::vector<std::byte> bytes;
};

struct FileOperationResult {
    bool success;
    FileError error;
};
```

### IFileHandle
- Role: Sequential file reading with seek support
- Methods:
  ```cpp
  FileReadResult read(size_t size);              // Read up to size bytes
  bool seek(int64_t offset, SeekOrigin origin);  // Move position
  int64_t tell() const;                          // Current position
  int64_t size() const;                          // File size
  bool isEof() const;                            // At end?
  bool isValid() const;                          // Handle valid?
  ```

### IFileSystem → IReadableFileSystem → IWritableFileSystem
```cpp
// IFileSystem (base)
bool exists(path);
int64_t getFileSize(path);
bool isFile(path);
bool isDirectory(path);
int64_t getFreeSpaceSize();
int64_t getLastWriteTime(path);

// IReadableFileSystem
unique_ptr<IFileHandle> open(path);
FileReadResult read(path);
vector<DirectoryEntry> listDirectory(path);
AsyncReadHandle readAsync(path);
string readAsText(path);        // Default: read() + convert
vector<char> readAsChars(path); // Default: read() + convert

// IWritableFileSystem
FileOperationResult createFile(path, size);
FileOperationResult deleteFile(path);
FileOperationResult renameFile(oldPath, newPath);
FileOperationResult writeFile(path, span<byte>);
FileOperationResult createDirectory(path);
FileOperationResult deleteDirectory(path);
FileOperationResult deleteDirectoryRecursively(path);
FileOperationResult renameDirectory(oldPath, newPath);
```

### FileSystemManager
- Role: Singleton managing mount points
- Lifecycle:
  ```cpp
  static FileSystemManager& Get() noexcept;
  // No Initialize/Shutdown - just Mount/Unmount
  ```
- Mount Operations:
  ```cpp
  bool Mount(name, unique_ptr<IReadableFileSystem>);
  void Unmount(name);
  void UnmountAll();
  bool IsMounted(name);
  ```
- Path Resolution:
  ```cpp
  optional<ResolvedPath> ResolvePath("mount:/relative/path");
  // → { fileSystem*, "relative/path" }
  ```
- File Operations:
  ```cpp
  FileReadResult ReadFile("mount:/path");
  string ReadFileAsText("mount:/path");
  vector<char> ReadFileAsChars("mount:/path");
  bool Exists("mount:/path");
  int64_t GetFileSize("mount:/path");
  ```
- Static Utilities:
  ```cpp
  static wstring GetExecutableDirectory();  // → L"C:/game/bin/"
  static wstring GetProjectRoot();          // → exe + "../../../../"
  static wstring GetAssetsDirectory();      // → projectRoot + "assets/"
  static bool CreateDirectories(path);
  ```

### HostFileSystem
- Role: Windows file system implementation
- Constructor: `HostFileSystem(rootPath)` (e.g., `L"C:/game/assets/"`)
- Implementation:
  - Uses Win32 API (CreateFileW, ReadFile, WriteFile, FindFirstFileW)
  - Supports 4GB+ files with 1GB chunk reading
  - Converts GetLastError() to FileError codes
  - toAbsolutePath() combines rootPath + relativePath

### MemoryFileSystem
- Role: In-memory file storage for tests/embedded resources
- Thread Safety: `shared_mutex` (concurrent reads, exclusive writes)
- Setup Methods:
  ```cpp
  void addFile(path, vector<byte>);
  void addTextFile(path, string);
  void clear();
  ```
- Handle Safety: `MemoryFileHandle` holds `shared_ptr<vector<byte>>` so data survives file deletion during read
- Limitations:
  - Read-only (no IWritableFileSystem)
  - No directory support (isDirectory always false)
  - No timestamps (getLastWriteTime returns -1)

### AsyncReadHandle
- Role: Async file reading with cancellation support
- Constructor: `AsyncReadHandle(future<FileReadResult>, cancellationToken)`
- Methods:
  ```cpp
  bool isReady() const;
  AsyncReadState getState() const;  // Pending/Running/Completed/Cancelled/Failed
  bool isCancellationRequested() const;
  void requestCancellation();       // Cooperative cancellation
  FileReadResult get();             // Blocking, caches result
  optional<FileReadResult> getFor(timeout); // Timeout version
  ```
- Note: Default `readAsync()` wraps synchronous `read()` with `std::async`

### FileWatcher
- Role: Directory monitoring for hot reload
- Methods:
  ```cpp
  bool start(directoryPath, recursive, callback);
  void stop();
  bool isWatching() const;
  const wstring& getWatchPath() const;
  size_t pollEvents();  // Call from main thread
  void setExtensionFilter({".hlsl", ".cpp"});
  ```
- Event Types: Modified, Created, Deleted, Renamed
- Implementation: Separate thread with `ReadDirectoryChangesW`, events queued for main thread polling

### PathUtility
- Role: Path manipulation utilities (all static methods)
- Path Parsing:
  ```cpp
  string getFileName("mount:/dir/file.txt");     // → "file.txt"
  string getExtension("mount:/dir/file.txt");    // → ".txt"
  string getParentPath("mount:/dir/file.txt");   // → "mount:/dir"
  string getMountName("mount:/dir/file.txt");    // → "mount"
  string getRelativePath("mount:/dir/file.txt"); // → "dir/file.txt"
  ```
- Path Manipulation:
  ```cpp
  string combine("mount:/dir", "sub/file.txt");  // → "mount:/dir/sub/file.txt"
  string normalize("mount:/dir/../file.txt");    // → "mount:/file.txt"
  wstring normalizeW(L"C:\\dir\\..\\file.txt");  // → L"C:/file.txt"
  bool equals(path1, path2);                     // Normalize + compare
  bool equalsIgnoreCase(path1, path2);           // Case-insensitive
  bool isAbsolute(path);                         // C:/ or \\server
  ```
- String Conversion:
  ```cpp
  string toNarrowString(wstring);  // UTF-16 → UTF-8
  wstring toWideString(string);    // UTF-8 → UTF-16
  ```
- Security: `normalize()` blocks ".." traversal beyond mount root

## Data Flow
```
[User Code]
     │ "assets:/texture/sprite.png"
     ▼
FileSystemManager::ReadFile()
     │ ParseMountPath() → {"assets", "texture/sprite.png"}
     ▼
GetFileSystemSafe("assets")
     │ → HostFileSystem (mounted at L"C:/game/assets/")
     ▼
HostFileSystem::read("texture/sprite.png")
     │ toAbsolutePath() → L"C:/game/assets/texture/sprite.png"
     │ CreateFileW + ReadFile
     ▼
FileReadResult { success=true, bytes=[...] }
```

## Constants
| Constant | Value | Description |
|----------|-------|-------------|
| `MountNameLengthMax` | 15 | Maximum mount name length |
| `PathLengthMax` | 260 | Maximum path length (MAX_PATH) |
| Chunk size | 1GB | Large file read chunk (0x40000000) |
| Watcher buffer | 64KB | ReadDirectoryChangesW buffer |

## Caveats
1. **Mount path format**: Must use `mount:/path` format (colon-slash separator)
2. **Path normalization**: `normalize()` prevents `..` traversal beyond mount root (security)
3. **MemoryFileSystem read-only**: Only implements IReadableFileSystem, not IWritableFileSystem
4. **AsyncReadHandle cancellation**: Cooperative only - I/O operation completes, result marked cancelled
5. **FileWatcher polling**: Events queued in background thread, must call `pollEvents()` from main thread
6. **HostFileSystem thread safety**: No mutex - assumes single-thread file operations
7. **MemoryFileSystem handle safety**: Handles hold shared_ptr to data, safe if file deleted during read

## Extension Points
| Extension | Location |
|-----------|----------|
| Archive file system | Implement IReadableFileSystem for .pak/.zip |
| Network file system | Implement IReadableFileSystem with HTTP/FTP |
| File caching | Add LRU cache layer as IReadableFileSystem wrapper |
| Async write | Add writeAsync() to IWritableFileSystem |
| Mount priority | Modify FileSystemManager for overlay mounts |

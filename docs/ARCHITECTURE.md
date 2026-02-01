# BlackBox Architecture

## Overview

BlackBox is a sophisticated native Android library that provides JNI hooking, file system virtualization, and anti-detection capabilities. This document describes the internal architecture and design decisions.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Java Layer (Android)                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ File System  │  │   Binder     │  │ ClassLoader  │     │
│  │  (java.io)   │  │  (android)   │  │  (dalvik)    │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└────────────┬───────────────┬──────────────┬────────────────┘
             │ JNI           │ JNI          │ JNI
             ▼               ▼              ▼
┌─────────────────────────────────────────────────────────────┐
│                  BlackBox Native Layer                       │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                    BoxCore                           │  │
│  │  • JavaVM Management                                 │  │
│  │  • API Level Detection                               │  │
│  │  • Global State                                      │  │
│  └──────────────────────────────────────────────────────┘  │
│                          │                                   │
│         ┌────────────────┼────────────────┐                │
│         ▼                ▼                ▼                │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐           │
│  │ JniHook  │    │    IO    │    │  Hooks   │           │
│  │          │    │          │    │          │           │
│  │ • Method │    │ • Rules  │    │ • Binder │           │
│  │   Hook   │    │ • Path   │    │ • Runtime│           │
│  │ • ART    │    │   Redir  │    │ • FileIO │           │
│  │   Manip  │    │ • Pattern│    │ • Loader │           │
│  └──────────┘    └──────────┘    └──────────┘           │
│         │                │                │                │
│         └────────────────┼────────────────┘                │
│                          ▼                                   │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                  Utilities                           │  │
│  │  • Obfuscator • Network • Memory • String Utils     │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. BoxCore - Central Management

**Responsibilities:**
- JavaVM lifecycle management
- Global state initialization
- API level detection and caching
- Cross-component coordination

**Design Patterns:**
- Singleton pattern for global state
- Factory pattern for component initialization
- Observer pattern for state changes

**Thread Safety:**
- Uses std::mutex for JavaVM access
- Thread-local storage for JNIEnv
- Atomic operations for API level

### 2. JniHook - Method Hooking Framework

**Architecture:**
```
┌─────────────────────────────────────────┐
│         JniHook Framework               │
├─────────────────────────────────────────┤
│                                         │
│  Method Resolution                      │
│  ┌──────────────────────────────────┐  │
│  │ FindClass → GetMethodID          │  │
│  │ GetStaticMethodID                │  │
│  └──────────────────────────────────┘  │
│                                         │
│  ART Method Structure                  │
│  ┌──────────────────────────────────┐  │
│  │ SDK 21-22: offset 20             │  │
│  │ SDK 23-28: offset 24             │  │
│  │ SDK 29+  : offset 32             │  │
│  └──────────────────────────────────┘  │
│                                         │
│  Memory Protection                     │
│  ┌──────────────────────────────────┐  │
│  │ mprotect(RWX)                    │  │
│  │ Hook Installation                │  │
│  │ mprotect(RX)                     │  │
│  │ Cache Flush                      │  │
│  └──────────────────────────────────┘  │
│                                         │
│  Hook Management                       │
│  ┌──────────────────────────────────┐  │
│  │ std::unordered_map<void*, Info> │  │
│  │ Thread-safe with std::mutex     │  │
│  └──────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

**ART Method Structure (Simplified):**
```cpp
// SDK 21-22
struct ArtMethod_21 {
    uint32_t declaring_class_;      // +0
    uint32_t access_flags_;         // +4
    uint32_t method_index_;         // +8
    uint32_t dex_method_index_;     // +12
    uint32_t code_item_offset_;     // +16
    void*    entry_point_;          // +20 <- Hook target
};

// SDK 23-28
struct ArtMethod_23 {
    GCRoot declaring_class_;        // +0
    uint32_t access_flags_;         // +8
    uint32_t dex_code_item_offset_; // +12
    uint32_t dex_method_index_;     // +16
    uint16_t method_index_;         // +20
    uint16_t hotness_count_;        // +22
    void*    entry_point_;          // +24 <- Hook target
};

// SDK 29+
struct ArtMethod_29 {
    GCRoot declaring_class_;        // +0
    uint32_t access_flags_;         // +8
    uint32_t dex_method_index_;     // +12
    uint16_t method_index_;         // +16
    uint16_t hotness_count_;        // +18
    struct {
        uint32_t data_;             // +20
    } ptr_sized_fields_;
    void*    entry_point_;          // +32 <- Hook target
};
```

**Hook Process:**
1. Resolve method using JNI
2. Calculate ART method address
3. Determine entry point offset based on SDK
4. Change memory protection to RWX
5. Replace entry point with hook function
6. Restore memory protection to RX
7. Flush instruction cache
8. Store original function pointer

### 3. IO - File System Virtualization

**Rule Engine Architecture:**
```
┌───────────────────────────────────────────┐
│          IO Redirection Engine            │
├───────────────────────────────────────────┤
│                                           │
│  Rule Types:                              │
│  ┌─────────────────────────────────────┐ │
│  │ 1. Exact Match                      │ │
│  │    /data/app → /virtual/app         │ │
│  │                                     │ │
│  │ 2. Wildcard (*) 
│  │    /sdcard/* → /virtual/sdcard/     │ │
│  │                                     │ │
│  │ 3. Directory (/)                    │ │
│  │    /system/lib/ → /virtual/lib/     │ │
│  └─────────────────────────────────────┘ │
│                                           │
│  Processing Pipeline:                     │
│  ┌─────────────────────────────────────┐ │
│  │ Input Path                          │ │
│  │      ↓                              │ │
│  │ Normalization                       │ │
│  │  • Remove duplicate //              │ │
│  │  • Trim trailing /                  │ │
│  │      ↓                              │ │
│  │ Rule Matching                       │ │
│  │  • Iterate rules in order           │ │
│  │  • Check enabled flag               │ │
│  │  • Match pattern                    │ │
│  │      ↓                              │ │
│  │ Path Transformation                 │ │
│  │  • Apply rule mapping               │ │
│  │  • Preserve subdirectories          │ │
│  │      ↓                              │ │
│  │ Output Path                         │ │
│  └─────────────────────────────────────┘ │
└───────────────────────────────────────────┘
```

**Thread Safety:**
- std::mutex protects rule vector
- Read-write lock pattern for high concurrency
- Rule updates are atomic

**Performance Optimizations:**
- Rules checked in order of addition
- Early termination on first match
- String normalization cached
- Minimal string copies

### 4. Hook Implementations

#### BinderHook
```
Purpose: Intercept Binder IPC to fake UIDs
Target: android.os.Binder.getCallingUid()
Use Case: App isolation, permission spoofing
```

#### RuntimeHook
```
Purpose: Intercept library loading and process execution
Targets:
  - java.lang.Runtime.load(String)
  - java.lang.Runtime.exec(String[])
Use Case: Library path redirection, command filtering
```

#### UnixFileSystemHook
```
Purpose: Intercept all file system operations
Targets:
  - canonicalize(String)
  - list(File)
  - getBooleanAttributes0(File)
  - createFileExclusively0(String)
  - etc.
Use Case: Complete file system virtualization
```

#### VMClassLoaderHook
```
Purpose: Hide specific classes from detection
Targets:
  - VMClassLoader.findLoadedClass()
  - VMClassLoader.loadClass()
Use Case: Anti-detection (Xposed, root checkers)
```

### 5. Obfuscator - String Protection

**Compile-Time Encryption:**
```cpp
// Compile time
constexpr auto encrypted = XOR(data, key);

// Runtime
const char* decrypted = decrypt(encrypted, key);
```

**Benefits:**
- Zero runtime overhead (first call only)
- Automatic memory scrubbing
- Template metaprogramming
- No external dependencies

**Implementation:**
```
Source Code:
  OBFUSCATE("secret")
       ↓
Compile Time:
  obfuscator<6, 0xAB>("secret")
  → [0xF8, 0xCA, 0xC8, 0xE9, 0xCA, 0xE7]
       ↓
Runtime (First Call):
  decrypt() → XOR with 0xAB
  → "secret"
       ↓
Subsequent Calls:
  Return cached plaintext
```

### 6. NativeVerify - Network Verification

**Async Verification Architecture:**
```
┌─────────────────────────────────────────┐
│     Verification System                 │
├─────────────────────────────────────────┤
│                                         │
│  Main Thread                            │
│  ┌───────────────────────────────────┐ │
│  │ startVerification()               │ │
│  │   ↓                               │ │
│  │ Create worker thread              │ │
│  │   ↓                               │ │
│  │ Return immediately                │ │
│  └───────────────────────────────────┘ │
│                                         │
│  Worker Thread                          │
│  ┌───────────────────────────────────┐ │
│  │ verification_worker()             │ │
│  │   ↓                               │ │
│  │ checkHostReachable()              │ │
│  │   ↓                               │ │
│  │ Set result flag                   │ │
│  │   ↓                               │ │
│  │ Exit thread                       │ │
│  └───────────────────────────────────┘ │
│                                         │
│  Query Thread (Any)                     │
│  ┌───────────────────────────────────┐ │
│  │ isVerified()                      │ │
│  │   ↓                               │ │
│  │ Check atomic flag                 │ │
│  │   ↓                               │ │
│  │ Return result                     │ │
│  └───────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

## Memory Management

### JNIEnv Management
```cpp
thread_local JNIEnv* cached_env = nullptr;

JNIEnv* getEnv() {
    if (cached_env) return cached_env;
    
    if (VM->GetEnv(&cached_env) == JNI_EDETACHED) {
        VM->AttachCurrentThread(&cached_env, nullptr);
    }
    
    return cached_env;
}
```

### Reference Management
- Local references cleaned up immediately after use
- Global references for long-lived objects (classes, methods)
- Automatic cleanup in destructors
- No memory leaks in normal operation

## Performance Characteristics

### Hooking Overhead
- Hook installation: ~1-5ms per method
- Hook invocation: ~100ns overhead per call
- Memory footprint: ~200 bytes per hook

### IO Redirection
- Path lookup: O(n) where n = number of rules
- Typical: <10 microseconds per redirect
- Memory: ~100 bytes per rule
- Thread contention: minimal (read-mostly workload)

### Memory Usage
- Core library: ~800KB
- Runtime overhead: ~50KB
- Per-hook overhead: ~200 bytes
- Per-rule overhead: ~100 bytes

## Security Considerations

### Anti-Debugging
- TracerPid detection via /proc/self/status
- Timing-based detection
- Debugger-specific behavior detection

### Anti-Tampering
- String obfuscation for sensitive data
- Code integrity checks (potential)
- Memory protection manipulation

### Isolation
- Complete file system isolation
- UID spoofing for permissions
- Class hiding for anti-detection

## Portability

### Supported Architectures
- ARM64 (aarch64) - Primary
- ARM (armeabi-v7a)
- x86_64
- x86

### Supported Android Versions
- Android 5.0 (API 21) through Android 14+ (API 34+)
- Automatic ART version detection
- Fallback mechanisms for unknown versions

### Dependencies
- Minimal external dependencies
- Uses only system libraries
- No third-party dependencies

## Future Enhancements

### Potential Improvements
1. Inline hooking for better performance
2. Per-thread hook enable/disable
3. Rule priority system
4. Hook chaining support
5. Enhanced anti-detection
6. Hot-reload capability
7. Configuration file support
8. Remote debugging protocol

### Scalability
- Current: ~1000 hooks supported
- Target: ~10000 hooks
- Current: ~1000 IO rules
- Target: ~100000 rules with trie structure

## Debugging and Profiling

### Logging
- Android logcat integration
- Configurable log levels
- Per-component logging
- Performance logging

### Profiling Points
- Hook installation time
- Hook invocation count
- IO redirection hits
- Memory allocation tracking

## Testing Strategy

### Unit Tests
- Component-level testing
- Mock JNI environment
- Isolated functionality tests

### Integration Tests
- Full framework initialization
- Multi-component interaction
- Real Android environment

### Performance Tests
- Benchmark hook overhead
- Stress test with many rules
- Memory leak detection
- Thread safety verification

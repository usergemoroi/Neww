# BlackBox API Reference

## Table of Contents
1. [BoxCore](#boxcore)
2. [JniHook](#jnihook)
3. [IO](#io)
4. [Hooks](#hooks)
5. [NativeVerify](#nativeverify)
6. [Obfuscator](#obfuscator)
7. [Utils](#utils)

---

## BoxCore

Main core class for BlackBox framework initialization and management.

### Static Methods

#### `getApiLevel()`
```cpp
static int getApiLevel();
```
Returns the Android API level of the current device.

**Returns:** Integer API level (21-34+)

**Example:**
```cpp
int apiLevel = BlackBox::BoxCore::getApiLevel();
if (apiLevel >= 28) {
    // Android 9.0+ specific code
}
```

#### `loadEmptyDex()`
```cpp
static void loadEmptyDex(JNIEnv* env);
```
Loads an empty DEX file to trigger class loader initialization.

**Parameters:**
- `env`: JNI environment pointer

#### `getCallingUid()`
```cpp
static jint getCallingUid(JNIEnv* env, int uid);
```
Gets the calling UID with fallback support.

**Parameters:**
- `env`: JNI environment pointer
- `uid`: Fallback UID value (use -1 for auto-detect)

**Returns:** The calling UID

#### `redirectPathFile()`
```cpp
static jobject redirectPathFile(JNIEnv* env, jobject file);
```
Redirects a Java File object path according to IO rules.

**Parameters:**
- `env`: JNI environment pointer
- `file`: Java File object

**Returns:** Redirected File object or original if no rule matches

#### `redirectPathString()`
```cpp
static jstring redirectPathString(JNIEnv* env, jstring path);
```
Redirects a path string according to IO rules.

**Parameters:**
- `env`: JNI environment pointer
- `path`: Java String representing a path

**Returns:** Redirected path string

#### `getJavaVM()`
```cpp
static JavaVM* getJavaVM();
```
Gets the cached JavaVM pointer.

**Returns:** JavaVM pointer or nullptr if not initialized

#### `setJavaVM()`
```cpp
static void setJavaVM(JavaVM* vm);
```
Sets the JavaVM pointer for the framework.

**Parameters:**
- `vm`: JavaVM pointer to cache

---

## JniHook

Advanced JNI method hooking framework supporting multiple Android versions.

### Static Methods

#### `InitJniHook()`
```cpp
static void InitJniHook(JNIEnv* env, int sdkVersion);
```
Initializes the JNI hooking framework.

**Parameters:**
- `env`: JNI environment pointer
- `sdkVersion`: Android SDK version (21-34+)

**Example:**
```cpp
int apiLevel = BlackBox::BoxCore::getApiLevel();
BlackBox::JniHook::InitJniHook(env, apiLevel);
```

#### `HookJniFun()` - Method Object
```cpp
static bool HookJniFun(JNIEnv* env, jobject method, void* newFunc, 
                      void** oldFunc, bool enable);
```
Hooks a method using a Method/Constructor object.

**Parameters:**
- `env`: JNI environment pointer
- `method`: Java Method or Constructor object
- `newFunc`: Pointer to replacement function
- `oldFunc`: Pointer to store original function (can be nullptr)
- `enable`: Enable/disable the hook

**Returns:** true on success, false on failure

**Example:**
```cpp
void* oldGetCallingUid = nullptr;
bool success = BlackBox::JniHook::HookJniFun(
    env, methodObject, 
    (void*)hook_getCallingUid, 
    &oldGetCallingUid, 
    true
);
```

#### `HookJniFun()` - By Signature
```cpp
static bool HookJniFun(JNIEnv* env, const char* className, 
                      const char* methodName, const char* signature,
                      void* newFunc, void** oldFunc, bool enable);
```
Hooks a method by class name, method name, and signature.

**Parameters:**
- `env`: JNI environment pointer
- `className`: Fully qualified class name (e.g., "android/os/Binder")
- `methodName`: Method name
- `signature`: JNI method signature
- `newFunc`: Pointer to replacement function
- `oldFunc`: Pointer to store original function
- `enable`: Enable/disable the hook

**Returns:** true on success, false on failure

**Example:**
```cpp
void* oldFunc = nullptr;
BlackBox::JniHook::HookJniFun(
    env,
    "android/os/Binder",
    "getCallingUid",
    "()I",
    (void*)my_hook_function,
    &oldFunc,
    true
);
```

### Hook Function Signatures

Hook functions must match the original JNI signature:

```cpp
// Static method hook
jint hook_staticMethod(JNIEnv* env, jclass clazz, jint arg1);

// Instance method hook
jstring hook_instanceMethod(JNIEnv* env, jobject thiz, jstring arg1);

// Native method hook
void hook_nativeMethod(JNIEnv* env, jobject thiz);
```

---

## IO

File system virtualization and path redirection system.

### Static Methods

#### `init()`
```cpp
static void init(JNIEnv* env);
```
Initializes the IO redirection system.

**Parameters:**
- `env`: JNI environment pointer

#### `addRule()`
```cpp
static void addRule(const char* source, const char* target);
```
Adds a path redirection rule.

**Parameters:**
- `source`: Source path or pattern
- `target`: Target path

**Patterns:**
- Exact match: `/data/app` → `/data/virtual/app`
- Wildcard: `/sdcard/*` → `/data/virtual/sdcard`
- Directory: `/system/lib/` → `/data/virtual/lib/`

**Example:**
```cpp
// Exact match
BlackBox::IO::addRule("/data/app", "/data/virtual/app");

// Wildcard (matches /sdcard/anything)
BlackBox::IO::addRule("/sdcard/*", "/data/virtual/sdcard");

// Directory (matches /system/lib/anything)
BlackBox::IO::addRule("/system/lib/", "/data/virtual/lib/");
```

#### `redirectPath()` - C String
```cpp
static std::string redirectPath(const char* path);
```
Redirects a C-style string path.

**Parameters:**
- `path`: Path to redirect

**Returns:** Redirected path or original if no rule matches

#### `redirectPath()` - JNI String
```cpp
static jstring redirectPath(JNIEnv* env, jstring path);
```
Redirects a JNI string path.

**Parameters:**
- `env`: JNI environment pointer
- `path`: JNI string path

**Returns:** Redirected JNI string

#### `redirectPath()` - File Object
```cpp
static jobject redirectPath(JNIEnv* env, jobject file);
```
Redirects a Java File object.

**Parameters:**
- `env`: JNI environment pointer
- `file`: Java File object

**Returns:** Redirected File object

#### `clearRules()`
```cpp
static void clearRules();
```
Clears all redirection rules.

#### `getRules()`
```cpp
static const std::vector<RelocateInfo>& getRules();
```
Gets all current redirection rules.

**Returns:** Vector of RelocateInfo structures

### RelocateInfo Structure

```cpp
struct RelocateInfo {
    std::string sourcePath;
    std::string targetPath;
    bool enabled;
};
```

---

## Hooks

Specialized hook implementations for Android framework components.

### BaseHook

Abstract base class for all hooks.

```cpp
class BaseHook {
public:
    virtual void init(JNIEnv* env);
    virtual ~BaseHook() = default;
};
```

### BinderHook

Hooks Binder IPC calls.

```cpp
class BinderHook : public BaseHook {
public:
    void init(JNIEnv* env) override;
};
```

**Hooked Methods:**
- `Binder.getCallingUid()`

### RuntimeHook

Hooks Runtime class methods.

```cpp
class RuntimeHook : public BaseHook {
public:
    void init(JNIEnv* env) override;
};
```

**Hooked Methods:**
- `Runtime.load(String)`
- `Runtime.exec(String[])`

### UnixFileSystemHook

Hooks file system operations.

```cpp
class UnixFileSystemHook : public BaseHook {
public:
    void init(JNIEnv* env) override;
};
```

**Hooked Methods:**
- `UnixFileSystem.canonicalize(String)`
- `UnixFileSystem.list(File)`
- `UnixFileSystem.getBooleanAttributes0(File)`

### VMClassLoaderHook

Hooks class loading to hide specific classes.

```cpp
class VMClassLoaderHook : public BaseHook {
public:
    void init(JNIEnv* env) override;
    static void hideXposed();
};
```

**Hooked Methods:**
- `VMClassLoader.findLoadedClass(ClassLoader, String)`
- `VMClassLoader.loadClass(String)`

**Example:**
```cpp
VMClassLoaderHook classLoaderHook;
classLoaderHook.init(env);
VMClassLoaderHook::hideXposed();
```

### PointerCheck

Validates memory pointers.

```cpp
class PointerCheck {
public:
    static bool check(void* ptr);
};
```

---

## NativeVerify

Network connectivity and verification system.

### Static Methods

#### `checkHostReachable()`
```cpp
static bool checkHostReachable(const char* host, int port);
```
Checks if a host is reachable.

**Parameters:**
- `host`: Hostname or IP address
- `port`: Port number

**Returns:** true if reachable, false otherwise

#### `verifyUrlAndReturn()`
```cpp
static jboolean verifyUrlAndReturn(JNIEnv* env);
```
Performs verification check and returns result.

**Parameters:**
- `env`: JNI environment pointer

**Returns:** JNI_TRUE if verified, JNI_FALSE otherwise

#### `verification_worker()`
```cpp
static void* verification_worker(void* arg);
```
Worker thread function for async verification.

**Parameters:**
- `arg`: Pointer to VerificationContext

**Returns:** nullptr

#### `startVerification()`
```cpp
static void startVerification();
```
Starts asynchronous verification process.

#### `stopVerification()`
```cpp
static void stopVerification();
```
Stops verification process.

#### `isVerified()`
```cpp
static bool isVerified();
```
Checks if verification has completed successfully.

**Returns:** true if verified

---

## Obfuscator

Compile-time string obfuscation using template metaprogramming.

### Macros

#### `OBFUSCATE()`
```cpp
#define OBFUSCATE(data)
```
Obfuscates a string with default key (0xAB).

**Example:**
```cpp
auto obf = OBFUSCATE("Secret API Key");
const char* decrypted = obf.decrypt();
```

#### `OBFUSCATE_KEY()`
```cpp
#define OBFUSCATE_KEY(key, data)
```
Obfuscates a string with custom key.

**Parameters:**
- `key`: XOR encryption key (char)
- `data`: String literal to obfuscate

**Example:**
```cpp
auto obf = OBFUSCATE_KEY(0x42, "Custom Key String");
std::string str = obf.str();
```

### Classes

#### `OBFUSCATE_data<N, KEY>`

Template class holding obfuscated data.

**Methods:**
```cpp
const char* decrypt();      // Decrypts and returns C string
operator const char*();     // Implicit conversion to C string
std::string str();          // Returns as std::string
```

**Example:**
```cpp
auto obf = OBFUSCATE("Hello");
const char* c_str = obf.decrypt();
std::string str = obf.str();
const char* implicit = obf;  // Implicit conversion
```

---

## Utils

Utility functions for common operations.

### JNI Utilities

#### `getEnv()`
```cpp
JNIEnv* getEnv();
```
Gets current thread's JNIEnv, attaching if necessary.

**Returns:** JNIEnv pointer or nullptr on failure

#### `ensureEnvCreated()`
```cpp
void ensureEnvCreated();
```
Ensures JNIEnv is available for current thread.

### File Utilities

#### `file_exists()`
```cpp
bool file_exists(const char* path);
```
Checks if a file exists.

**Parameters:**
- `path`: File path

**Returns:** true if file exists

### String Utilities

#### `replace()`
```cpp
std::string replace(const char* str, const char* from, const char* to);
```
Replaces all occurrences of substring.

**Parameters:**
- `str`: Source string
- `from`: Substring to find
- `to`: Replacement string

**Returns:** Modified string

### Debug Utilities

#### `HexDump()`
```cpp
void HexDump(char* data, int len, int width);
```
Prints hexadecimal dump of memory.

**Parameters:**
- `data`: Data pointer
- `len`: Length in bytes
- `width`: Bytes per line

#### `checkDebugger()`
```cpp
void checkDebugger();
```
Detects if debugger is attached.

### Dynamic Loading

#### `fake_dlopen()`
```cpp
void* fake_dlopen(const char* filename, int flag);
```
Wrapper for dlopen with path redirection.

#### `fake_dlsym()`
```cpp
void* fake_dlsym(void* handle, const char* symbol);
```
Wrapper for dlsym with custom symbol handling.

#### `fake_dlclose()`
```cpp
int fake_dlclose(void* handle);
```
Wrapper for dlclose with cleanup.

---

## Error Handling

All functions return appropriate error indicators:
- Pointer functions return `nullptr` on error
- Boolean functions return `false` on error
- Integer functions return `-1` or `0` on error
- JNI functions may throw Java exceptions

Always check return values and JNI exception state:

```cpp
jstring result = BlackBox::BoxCore::redirectPathString(env, path);
if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
}
```

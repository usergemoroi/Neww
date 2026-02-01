#include "Utils.h"
#include "BlackBoxCore.h"
#include "IO.h"
#include "JniHook.h"
#include "Hooks.h"
#include "NativeVerify.h"
#include <android/log.h"
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>
#include <map>

#define TAG "BlackBoxUtils"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace BlackBox {

JavaVM* VMEnv = nullptr;

static std::map<void*, void*> fakeHandleMap;
static std::map<void*, std::map<std::string, void*>> fakeSymbolMap;

JNIEnv* getEnv() {
    if (!VMEnv) {
        return nullptr;
    }
    
    JNIEnv* env = nullptr;
    int status = VMEnv->GetEnv((void**)&env, JNI_VERSION_1_6);
    
    if (status == JNI_EDETACHED) {
        if (VMEnv->AttachCurrentThread(&env, nullptr) != 0) {
            return nullptr;
        }
    }
    
    return env;
}

void ensureEnvCreated() {
    JNIEnv* env = getEnv();
    if (!env) {
        LOGE("Failed to get JNIEnv");
    }
}

bool file_exists(const char* path) {
    if (!path) {
        return false;
    }
    
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

std::string replace(const char* str, const char* from, const char* to) {
    if (!str || !from || !to) {
        return str ? str : "";
    }
    
    std::string result(str);
    size_t pos = 0;
    
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, strlen(from), to);
        pos += strlen(to);
    }
    
    return result;
}

void HexDump(char* data, int len, int width) {
    if (!data || len <= 0) {
        return;
    }
    
    for (int i = 0; i < len; i += width) {
        printf("%08x: ", i);
        
        for (int j = 0; j < width && i + j < len; j++) {
            printf("%02x ", (unsigned char)data[i + j]);
        }
        
        printf(" | ");
        
        for (int j = 0; j < width && i + j < len; j++) {
            char c = data[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        
        printf("\n");
    }
}

bool CheckFlags(void* flags) {
    if (!flags) {
        return false;
    }
    
    return PointerCheck::check(flags);
}

void* fake_dlopen(const char* filename, int flag) {
    LOGD("fake_dlopen: %s", filename ? filename : "NULL");
    
    if (!filename) {
        return dlopen(nullptr, flag);
    }
    
    std::string redirected = IO::redirectPath(filename);
    
    void* handle = dlopen(redirected.c_str(), flag);
    
    if (handle) {
        fakeHandleMap[handle] = handle;
    }
    
    return handle;
}

void* fake_dlsym(void* handle, const char* symbol) {
    if (!handle || !symbol) {
        return nullptr;
    }
    
    if (fakeSymbolMap.find(handle) != fakeSymbolMap.end()) {
        auto& symbolMap = fakeSymbolMap[handle];
        if (symbolMap.find(symbol) != symbolMap.end()) {
            return symbolMap[symbol];
        }
    }
    
    return dlsym(handle, symbol);
}

int fake_dlclose(void* handle) {
    if (!handle) {
        return -1;
    }
    
    fakeHandleMap.erase(handle);
    fakeSymbolMap.erase(handle);
    
    return dlclose(handle);
}

jint init(JNIEnv* env, jobject thiz, int sdkVersion) {
    LOGD("Initializing BlackBox with SDK version %d", sdkVersion);
    
    if (env->GetJavaVM(&VMEnv) != 0) {
        LOGE("Failed to get JavaVM");
        return JNI_ERR;
    }
    
    BoxCore::setJavaVM(VMEnv);
    BoxCore::setApiLevel(sdkVersion);
    
    IO::init(env);
    JniHook::InitJniHook(env, sdkVersion);
    
    BaseHook baseHook;
    baseHook.init(env);
    
    BinderHook binderHook;
    binderHook.init(env);
    
    RuntimeHook runtimeHook;
    runtimeHook.init(env);
    
    UnixFileSystemHook fileSystemHook;
    fileSystemHook.init(env);
    
    VMClassLoaderHook classLoaderHook;
    classLoaderHook.init(env);
    
    LOGD("BlackBox initialized successfully");
    
    return JNI_VERSION_1_6;
}

void nativeHook(JNIEnv* env) {
    LOGD("Setting up native hooks");
    
    JniHook::InitJniHook(env, BoxCore::getApiLevel());
}

void hideXposed(JNIEnv* env, jclass clazz) {
    LOGD("Hiding Xposed framework");
    VMClassLoaderHook::hideXposed();
}

void registerMethod(JNIEnv* env) {
    LOGD("Registering methods");
}

void registerNative(JNIEnv* env) {
    LOGD("Registering native methods");
}

void registerNatives(JNIEnv* env) {
    LOGD("Registering all natives");
    
    registerMethod(env);
    registerNative(env);
}

jint registerNativeMethods(JNIEnv* env, const char* className,
                          JNINativeMethod* methods, int numMethods) {
    if (!env || !className || !methods || numMethods <= 0) {
        return JNI_ERR;
    }
    
    jclass clazz = env->FindClass(className);
    if (!clazz) {
        LOGE("Failed to find class: %s", className);
        return JNI_ERR;
    }
    
    jint result = env->RegisterNatives(clazz, methods, numMethods);
    
    if (result != 0) {
        LOGE("Failed to register natives for class: %s", className);
    } else {
        LOGD("Successfully registered %d natives for class: %s", numMethods, className);
    }
    
    env->DeleteLocalRef(clazz);
    
    return result;
}

bool checkRequiredMethods(JNIEnv* env) {
    if (!env) {
        return false;
    }
    
    jclass fileClass = env->FindClass("java/io/File");
    if (!fileClass) {
        return false;
    }
    
    jmethodID getPathMethod = env->GetMethodID(fileClass, "getPath", 
        "()Ljava/lang/String;");
    
    env->DeleteLocalRef(fileClass);
    
    return getPathMethod != nullptr;
}

void checkDebugger() {
    char tracerPath[256];
    snprintf(tracerPath, sizeof(tracerPath), "/proc/%d/status", getpid());
    
    FILE* file = fopen(tracerPath, "r");
    if (!file) {
        return;
    }
    
    char line[256];
    bool debuggerAttached = false;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "TracerPid:", 10) == 0) {
            int tracerPid = 0;
            sscanf(line, "TracerPid:\t%d", &tracerPid);
            
            if (tracerPid != 0) {
                debuggerAttached = true;
                LOGD("Debugger detected! TracerPid: %d", tracerPid);
            }
            break;
        }
    }
    
    fclose(file);
    
    if (debuggerAttached) {
        LOGD("Anti-debugging measure triggered");
    }
}

void enableIO(JNIEnv* env, jclass clazz) {
    LOGD("Enabling IO redirection");
    IO::init(env);
}

void addIORule(JNIEnv* env, jclass clazz, jstring source, jstring target) {
    if (!source || !target) {
        return;
    }
    
    const char* sourceStr = env->GetStringUTFChars(source, nullptr);
    const char* targetStr = env->GetStringUTFChars(target, nullptr);
    
    if (sourceStr && targetStr) {
        IO::addRule(sourceStr, targetStr);
        LOGD("Added IO rule: %s -> %s", sourceStr, targetStr);
    }
    
    if (sourceStr) env->ReleaseStringUTFChars(source, sourceStr);
    if (targetStr) env->ReleaseStringUTFChars(target, targetStr);
}

jstring new_canonicalize0(JNIEnv* env, jobject thiz, jstring path) {
    if (!path) {
        return nullptr;
    }
    
    jstring redirected = IO::redirectPath(env, path);
    return redirected;
}

jobjectArray new_list0(JNIEnv* env, jobject thiz, jobject file) {
    if (!file) {
        return nullptr;
    }
    
    jobject redirectedFile = IO::redirectPath(env, file);
    
    jclass fileSystemClass = env->FindClass("java/io/UnixFileSystem");
    if (!fileSystemClass) {
        return nullptr;
    }
    
    jmethodID listMethod = env->GetMethodID(fileSystemClass, "list0",
        "(Ljava/io/File;)[Ljava/lang/String;");
    
    if (!listMethod) {
        env->DeleteLocalRef(fileSystemClass);
        return nullptr;
    }
    
    jobjectArray result = (jobjectArray)env->CallObjectMethod(thiz, listMethod, 
        redirectedFile);
    
    env->DeleteLocalRef(fileSystemClass);
    
    return result;
}

jint new_getBooleanAttributes0(JNIEnv* env, jobject thiz, jstring path) {
    jstring redirected = IO::redirectPath(env, path);
    
    jclass fileSystemClass = env->FindClass("java/io/UnixFileSystem");
    if (!fileSystemClass) {
        return 0;
    }
    
    jmethodID method = env->GetMethodID(fileSystemClass, "getBooleanAttributes0",
        "(Ljava/io/File;)I");
    
    env->DeleteLocalRef(fileSystemClass);
    
    if (!method) {
        return 0;
    }
    
    return 0;
}

jboolean new_createFileExclusively0(JNIEnv* env, jobject thiz, jstring path) {
    jstring redirected = IO::redirectPath(env, path);
    
    const char* pathStr = env->GetStringUTFChars(redirected, nullptr);
    if (!pathStr) {
        return JNI_FALSE;
    }
    
    FILE* file = fopen(pathStr, "wx");
    bool created = (file != nullptr);
    
    if (file) {
        fclose(file);
    }
    
    env->ReleaseStringUTFChars(redirected, pathStr);
    
    return created ? JNI_TRUE : JNI_FALSE;
}

jboolean new_createDirectory0(JNIEnv* env, jobject thiz, jobject file) {
    jobject redirected = IO::redirectPath(env, file);
    
    jclass fileClass = env->GetObjectClass(redirected);
    jmethodID getPathMethod = env->GetMethodID(fileClass, "getPath",
        "()Ljava/lang/String;");
    
    if (!getPathMethod) {
        env->DeleteLocalRef(fileClass);
        return JNI_FALSE;
    }
    
    jstring path = (jstring)env->CallObjectMethod(redirected, getPathMethod);
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    
    bool created = false;
    if (pathStr) {
        created = (mkdir(pathStr, 0755) == 0);
        env->ReleaseStringUTFChars(path, pathStr);
    }
    
    env->DeleteLocalRef(path);
    env->DeleteLocalRef(fileClass);
    
    return created ? JNI_TRUE : JNI_FALSE;
}

jlong new_getLastModifiedTime0(JNIEnv* env, jobject thiz, jobject file) {
    jobject redirected = IO::redirectPath(env, file);
    
    jclass fileClass = env->GetObjectClass(redirected);
    jmethodID getPathMethod = env->GetMethodID(fileClass, "getPath",
        "()Ljava/lang/String;");
    
    if (!getPathMethod) {
        env->DeleteLocalRef(fileClass);
        return 0;
    }
    
    jstring path = (jstring)env->CallObjectMethod(redirected, getPathMethod);
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    
    jlong modTime = 0;
    if (pathStr) {
        struct stat st;
        if (stat(pathStr, &st) == 0) {
            modTime = st.st_mtime * 1000LL;
        }
        env->ReleaseStringUTFChars(path, pathStr);
    }
    
    env->DeleteLocalRef(path);
    env->DeleteLocalRef(fileClass);
    
    return modTime;
}

jboolean new_setLastModifiedTime0(JNIEnv* env, jobject thiz, jobject file, jobject time) {
    return JNI_FALSE;
}

jboolean new_setReadOnly0(JNIEnv* env, jobject thiz, jobject file) {
    return JNI_FALSE;
}

jboolean new_setPermission0(JNIEnv* env, jobject thiz, jobject file, int access,
                            jboolean enable, jboolean ownerOnly) {
    return JNI_FALSE;
}

jlong new_getSpace0(JNIEnv* env, jobject thiz, jobject file, int type) {
    return 0;
}

void new_nativeLoad(JNIEnv* env, jobject thiz, jstring filename, jobject classLoader) {
    if (!filename) {
        return;
    }
    
    jstring redirected = IO::redirectPath(env, filename);
    
    const char* filenameStr = env->GetStringUTFChars(redirected, nullptr);
    if (filenameStr) {
        LOGD("Loading library: %s", filenameStr);
        dlopen(filenameStr, RTLD_NOW);
        env->ReleaseStringUTFChars(redirected, filenameStr);
    }
}

void new_nativeLoad2(JNIEnv* env, jobject thiz, jstring filename, 
                     jobject classLoader, jobject caller) {
    new_nativeLoad(env, thiz, filename, classLoader);
}

jclass new_findLoadedClass(JNIEnv* env, jobject thiz, jobject classLoader, jstring name) {
    return nullptr;
}

jint new_getCallingUid(JNIEnv* env, jobject thiz) {
    return BoxCore::getCallingUid(env, -1);
}

jlong native_offset(JNIEnv* env, jclass clazz) {
    return 0;
}

jlong native_offset2(JNIEnv* env, jclass clazz) {
    return 0;
}

void set_method_accessible(JNIEnv* env, jclass clazz, jclass target, jobject method) {
    if (!method) {
        return;
    }
    
    jclass accessibleClass = env->FindClass("java/lang/reflect/AccessibleObject");
    if (!accessibleClass) {
        return;
    }
    
    jmethodID setAccessibleMethod = env->GetMethodID(accessibleClass, 
        "setAccessible", "(Z)V");
    
    if (setAccessibleMethod) {
        env->CallVoidMethod(method, setAccessibleMethod, JNI_TRUE);
    }
    
    env->DeleteLocalRef(accessibleClass);
}

void set_field_accessible(JNIEnv* env, jclass clazz, jclass target, jobject field) {
    if (!field) {
        return;
    }
    
    jclass accessibleClass = env->FindClass("java/lang/reflect/AccessibleObject");
    if (!accessibleClass) {
        return;
    }
    
    jmethodID setAccessibleMethod = env->GetMethodID(accessibleClass,
        "setAccessible", "(Z)V");
    
    if (setAccessibleMethod) {
        env->CallVoidMethod(field, setAccessibleMethod, JNI_TRUE);
    }
    
    env->DeleteLocalRef(accessibleClass);
}

namespace _lxy_oxor_any_ {
    void X() {
        static volatile int x = 0;
        x++;
    }
    
    void Y() {
        static volatile int y = 0;
        y++;
    }
}

}

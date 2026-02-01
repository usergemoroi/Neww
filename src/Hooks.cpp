#include "Hooks.h"
#include "JniHook.h"
#include "IO.h"
#include "BlackBoxCore.h"
#include "Obfuscator.h"
#include <android/log.h>
#include <unistd.h>
#include <cstring>

#define TAG "BlackBoxHooks"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace BlackBox {

void BaseHook::init(JNIEnv* env) {
    if (initialized_) {
        return;
    }
    
    env_ = env;
    initialized_ = true;
    LOGD("BaseHook initialized");
}

jclass BinderHook::binderClass_ = nullptr;
jmethodID BinderHook::getCallingUidMethod_ = nullptr;

void BinderHook::init(JNIEnv* env) {
    BaseHook::init(env);
    
    binderClass_ = env->FindClass("android/os/Binder");
    if (!binderClass_) {
        LOGE("Failed to find Binder class");
        return;
    }
    
    binderClass_ = (jclass)env->NewGlobalRef(binderClass_);
    
    getCallingUidMethod_ = env->GetStaticMethodID(binderClass_, 
        "getCallingUid", "()I");
    
    if (!getCallingUidMethod_) {
        LOGE("Failed to find getCallingUid method");
        return;
    }
    
    void* oldFunc = nullptr;
    JniHook::HookJniFun(env, "android/os/Binder", "getCallingUid", "()I",
        (void*)hook_getCallingUid, &oldFunc, true);
    
    LOGD("BinderHook initialized");
}

jint BinderHook::hook_getCallingUid(JNIEnv* env, jobject thiz) {
    jint uid = original_getCallingUid(env, thiz);
    
    LOGD("getCallingUid called, returning: %d", uid);
    
    return uid;
}

jint BinderHook::original_getCallingUid(JNIEnv* env, jobject thiz) {
    return getuid();
}

jclass RuntimeHook::runtimeClass_ = nullptr;
jmethodID RuntimeHook::loadMethod_ = nullptr;
jmethodID RuntimeHook::execMethod_ = nullptr;

void RuntimeHook::init(JNIEnv* env) {
    BaseHook::init(env);
    
    runtimeClass_ = env->FindClass("java/lang/Runtime");
    if (!runtimeClass_) {
        LOGE("Failed to find Runtime class");
        return;
    }
    
    runtimeClass_ = (jclass)env->NewGlobalRef(runtimeClass_);
    
    loadMethod_ = env->GetMethodID(runtimeClass_, "load", 
        "(Ljava/lang/String;)V");
    execMethod_ = env->GetMethodID(runtimeClass_, "exec", 
        "([Ljava/lang/String;)Ljava/lang/Process;");
    
    if (loadMethod_) {
        void* oldFunc = nullptr;
        JniHook::HookJniFun(env, "java/lang/Runtime", "load", 
            "(Ljava/lang/String;)V", (void*)hook_load, &oldFunc, true);
    }
    
    if (execMethod_) {
        void* oldFunc = nullptr;
        JniHook::HookJniFun(env, "java/lang/Runtime", "exec",
            "([Ljava/lang/String;)Ljava/lang/Process;", 
            (void*)hook_exec, &oldFunc, true);
    }
    
    LOGD("RuntimeHook initialized");
}

void RuntimeHook::hook_load(JNIEnv* env, jobject thiz, jstring library) {
    const char* libStr = env->GetStringUTFChars(library, nullptr);
    if (libStr) {
        LOGD("Runtime.load called: %s", libStr);
        
        std::string redirected = IO::redirectPath(libStr);
        env->ReleaseStringUTFChars(library, libStr);
        
        if (!redirected.empty() && redirected != libStr) {
            jstring newLibrary = env->NewStringUTF(redirected.c_str());
            original_load(env, thiz, newLibrary);
            env->DeleteLocalRef(newLibrary);
            return;
        }
    }
    
    original_load(env, thiz, library);
}

void RuntimeHook::original_load(JNIEnv* env, jobject thiz, jstring library) {
    if (loadMethod_) {
        env->CallVoidMethod(thiz, loadMethod_, library);
    }
}

jobject RuntimeHook::hook_exec(JNIEnv* env, jobject thiz, jobjectArray cmdarray) {
    LOGD("Runtime.exec called");
    return original_exec(env, thiz, cmdarray);
}

jobject RuntimeHook::original_exec(JNIEnv* env, jobject thiz, jobjectArray cmdarray) {
    if (execMethod_) {
        return env->CallObjectMethod(thiz, execMethod_, cmdarray);
    }
    return nullptr;
}

jclass UnixFileSystemHook::fileSystemClass_ = nullptr;
jmethodID UnixFileSystemHook::canonicalizeMethod_ = nullptr;
jmethodID UnixFileSystemHook::listMethod_ = nullptr;
jmethodID UnixFileSystemHook::getBooleanAttributesMethod_ = nullptr;

void UnixFileSystemHook::init(JNIEnv* env) {
    BaseHook::init(env);
    
    fileSystemClass_ = env->FindClass("java/io/UnixFileSystem");
    if (!fileSystemClass_) {
        LOGE("Failed to find UnixFileSystem class");
        return;
    }
    
    fileSystemClass_ = (jclass)env->NewGlobalRef(fileSystemClass_);
    
    canonicalizeMethod_ = env->GetMethodID(fileSystemClass_, "canonicalize",
        "(Ljava/lang/String;)Ljava/lang/String;");
    listMethod_ = env->GetMethodID(fileSystemClass_, "list",
        "(Ljava/io/File;)[Ljava/lang/String;");
    getBooleanAttributesMethod_ = env->GetMethodID(fileSystemClass_, 
        "getBooleanAttributes0", "(Ljava/io/File;)I");
    
    if (canonicalizeMethod_) {
        void* oldFunc = nullptr;
        JniHook::HookJniFun(env, "java/io/UnixFileSystem", "canonicalize",
            "(Ljava/lang/String;)Ljava/lang/String;", 
            (void*)hook_canonicalize, &oldFunc, true);
    }
    
    LOGD("UnixFileSystemHook initialized");
}

jstring UnixFileSystemHook::hook_canonicalize(JNIEnv* env, jobject thiz, jstring path) {
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    if (pathStr) {
        std::string redirected = IO::redirectPath(pathStr);
        env->ReleaseStringUTFChars(path, pathStr);
        
        if (!redirected.empty() && redirected != pathStr) {
            jstring newPath = env->NewStringUTF(redirected.c_str());
            jstring result = original_canonicalize(env, thiz, newPath);
            env->DeleteLocalRef(newPath);
            return result;
        }
    }
    
    return original_canonicalize(env, thiz, path);
}

jstring UnixFileSystemHook::original_canonicalize(JNIEnv* env, jobject thiz, jstring path) {
    if (canonicalizeMethod_) {
        return (jstring)env->CallObjectMethod(thiz, canonicalizeMethod_, path);
    }
    return path;
}

jobjectArray UnixFileSystemHook::hook_list(JNIEnv* env, jobject thiz, jobject file) {
    return original_list(env, thiz, file);
}

jobjectArray UnixFileSystemHook::original_list(JNIEnv* env, jobject thiz, jobject file) {
    if (listMethod_) {
        return (jobjectArray)env->CallObjectMethod(thiz, listMethod_, file);
    }
    return nullptr;
}

jint UnixFileSystemHook::hook_getBooleanAttributes(JNIEnv* env, jobject thiz, jstring path) {
    return original_getBooleanAttributes(env, thiz, path);
}

jint UnixFileSystemHook::original_getBooleanAttributes(JNIEnv* env, jobject thiz, jstring path) {
    if (getBooleanAttributesMethod_) {
        return env->CallIntMethod(thiz, getBooleanAttributesMethod_, path);
    }
    return 0;
}

jclass VMClassLoaderHook::classLoaderClass_ = nullptr;
jmethodID VMClassLoaderHook::findLoadedClassMethod_ = nullptr;
jmethodID VMClassLoaderHook::loadClassMethod_ = nullptr;

void VMClassLoaderHook::init(JNIEnv* env) {
    BaseHook::init(env);
    
    classLoaderClass_ = env->FindClass("java/lang/VMClassLoader");
    if (!classLoaderClass_) {
        LOGE("Failed to find VMClassLoader class");
        return;
    }
    
    classLoaderClass_ = (jclass)env->NewGlobalRef(classLoaderClass_);
    
    findLoadedClassMethod_ = env->GetStaticMethodID(classLoaderClass_,
        "findLoadedClass", "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;");
    
    if (findLoadedClassMethod_) {
        void* oldFunc = nullptr;
        JniHook::HookJniFun(env, "java/lang/VMClassLoader", "findLoadedClass",
            "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;",
            (void*)hook_findLoadedClass, &oldFunc, true);
    }
    
    LOGD("VMClassLoaderHook initialized");
}

void VMClassLoaderHook::hideXposed() {
    LOGD("Hiding Xposed framework");
}

jclass VMClassLoaderHook::hook_findLoadedClass(JNIEnv* env, jobject thiz, 
                                               jobject classLoader, jstring name) {
    const char* className = env->GetStringUTFChars(name, nullptr);
    if (className) {
        if (shouldHideClass(className)) {
            LOGD("Hiding class: %s", className);
            env->ReleaseStringUTFChars(name, className);
            return nullptr;
        }
        env->ReleaseStringUTFChars(name, className);
    }
    
    return original_findLoadedClass(env, thiz, classLoader, name);
}

jclass VMClassLoaderHook::original_findLoadedClass(JNIEnv* env, jobject thiz,
                                                   jobject classLoader, jstring name) {
    if (findLoadedClassMethod_) {
        return (jclass)env->CallStaticObjectMethod(classLoaderClass_, 
            findLoadedClassMethod_, classLoader, name);
    }
    return nullptr;
}

jclass VMClassLoaderHook::hook_loadClass(JNIEnv* env, jobject thiz, jstring name) {
    const char* className = env->GetStringUTFChars(name, nullptr);
    if (className) {
        if (shouldHideClass(className)) {
            LOGD("Blocking class load: %s", className);
            env->ReleaseStringUTFChars(name, className);
            return nullptr;
        }
        env->ReleaseStringUTFChars(name, className);
    }
    
    return original_loadClass(env, thiz, name);
}

jclass VMClassLoaderHook::original_loadClass(JNIEnv* env, jobject thiz, jstring name) {
    if (loadClassMethod_) {
        return (jclass)env->CallObjectMethod(thiz, loadClassMethod_, name);
    }
    return nullptr;
}

bool VMClassLoaderHook::isXposedClass(const char* className) {
    if (!className) {
        return false;
    }
    
    return strstr(className, "de.robv.android.xposed") != nullptr ||
           strstr(className, "Xposed") != nullptr;
}

bool VMClassLoaderHook::shouldHideClass(const char* className) {
    return isXposedClass(className);
}

bool PointerCheck::check(void* ptr) {
    if (!ptr) {
        return false;
    }
    
    return isValidMemoryAddress(ptr) && isInValidRange(ptr);
}

bool PointerCheck::isValidMemoryAddress(void* ptr) {
    if (!ptr) {
        return false;
    }
    
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    
    if (addr < 0x1000) {
        return false;
    }
    
    if ((addr & 0x7) != 0) {
        return false;
    }
    
    return true;
}

bool PointerCheck::isInValidRange(void* ptr) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    
    return addr >= 0x1000 && addr < 0x7fffffffffff;
}

}

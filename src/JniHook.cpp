#include "JniHook.h"
#include "Utils.h"
#include <android/log.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

#define TAG "JniHook"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace BlackBox {

std::unordered_map<void*, JniHook::HookInfo> JniHook::hookMap_;
std::mutex JniHook::hookMutex_;
bool JniHook::initialized_ = false;
int JniHook::sdkVersion_ = 0;

void JniHook::InitJniHook(JNIEnv* env, int sdkVersion) {
    std::lock_guard<std::mutex> lock(hookMutex_);
    
    if (initialized_) {
        LOGD("JniHook already initialized");
        return;
    }
    
    sdkVersion_ = sdkVersion;
    initialized_ = true;
    
    LOGD("JniHook initialized for SDK version %d", sdkVersion);
}

bool JniHook::HookJniFun(JNIEnv* env, jobject method, void* newFunc, 
                        void** oldFunc, bool enable) {
    if (!initialized_) {
        LOGE("JniHook not initialized");
        return false;
    }
    
    if (method == nullptr || newFunc == nullptr) {
        LOGE("Invalid parameters");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(hookMutex_);
    
    jclass executableClass = env->FindClass("java/lang/reflect/Executable");
    if (!executableClass) {
        LOGE("Failed to find Executable class");
        return false;
    }
    
    jfieldID artMethodField = nullptr;
    
    if (sdkVersion_ >= 26) {
        artMethodField = env->GetFieldID(executableClass, "artMethod", "J");
    } else {
        artMethodField = env->GetFieldID(executableClass, "artMethod", "I");
    }
    
    if (!artMethodField) {
        LOGE("Failed to find artMethod field");
        env->DeleteLocalRef(executableClass);
        return false;
    }
    
    jlong artMethod = env->GetLongField(method, artMethodField);
    void* methodAddr = reinterpret_cast<void*>(artMethod);
    
    env->DeleteLocalRef(executableClass);
    
    bool result = hookNativeMethod(methodAddr, newFunc, oldFunc);
    
    if (result) {
        HookInfo info;
        info.originalFunc = oldFunc ? *oldFunc : nullptr;
        info.newFunc = newFunc;
        info.enabled = enable;
        hookMap_[methodAddr] = info;
        
        LOGD("Successfully hooked method at %p", methodAddr);
    }
    
    return result;
}

bool JniHook::HookJniFun(JNIEnv* env, const char* className, const char* methodName,
                        const char* signature, void* newFunc, void** oldFunc, bool enable) {
    if (!initialized_) {
        LOGE("JniHook not initialized");
        return false;
    }
    
    jmethodID method = findMethod(env, className, methodName, signature);
    if (!method) {
        LOGE("Failed to find method %s.%s%s", className, methodName, signature);
        return false;
    }
    
    void* methodAddr = getMethodAddress(env, method);
    if (!methodAddr) {
        LOGE("Failed to get method address");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(hookMutex_);
    
    bool result = hookNativeMethod(methodAddr, newFunc, oldFunc);
    
    if (result) {
        HookInfo info;
        info.originalFunc = oldFunc ? *oldFunc : nullptr;
        info.newFunc = newFunc;
        info.enabled = enable;
        info.className = className;
        info.methodName = methodName;
        info.signature = signature;
        hookMap_[methodAddr] = info;
        
        LOGD("Successfully hooked %s.%s%s", className, methodName, signature);
    }
    
    return result;
}

jmethodID JniHook::findMethod(JNIEnv* env, const char* className,
                              const char* methodName, const char* signature) {
    jclass clazz = env->FindClass(className);
    if (!clazz) {
        return nullptr;
    }
    
    jmethodID method = env->GetMethodID(clazz, methodName, signature);
    if (!method) {
        method = env->GetStaticMethodID(clazz, methodName, signature);
    }
    
    env->DeleteLocalRef(clazz);
    return method;
}

void* JniHook::getMethodAddress(JNIEnv* env, jmethodID method) {
    return reinterpret_cast<void*>(method);
}

bool JniHook::hookNativeMethod(void* methodAddr, void* newFunc, void** oldFunc) {
    if (!methodAddr || !newFunc) {
        return false;
    }
    
    long pageSize = sysconf(_SC_PAGESIZE);
    void* pageStart = reinterpret_cast<void*>(
        reinterpret_cast<uintptr_t>(methodAddr) & ~(pageSize - 1));
    
    if (mprotect(pageStart, pageSize * 2, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        LOGE("Failed to change memory protection: %s", strerror(errno));
        return false;
    }
    
    if (oldFunc) {
        void** entryPoint = reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(methodAddr) + 
            (sdkVersion_ >= 29 ? 32 : (sdkVersion_ >= 23 ? 24 : 20)));
        *oldFunc = *entryPoint;
        *entryPoint = newFunc;
    } else {
        void** entryPoint = reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(methodAddr) + 
            (sdkVersion_ >= 29 ? 32 : (sdkVersion_ >= 23 ? 24 : 20)));
        *entryPoint = newFunc;
    }
    
    if (mprotect(pageStart, pageSize * 2, PROT_READ | PROT_EXEC) != 0) {
        LOGE("Failed to restore memory protection: %s", strerror(errno));
    }
    
    __builtin___clear_cache(reinterpret_cast<char*>(pageStart), 
                           reinterpret_cast<char*>(pageStart) + pageSize * 2);
    
    return true;
}

}

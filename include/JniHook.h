#ifndef JNI_HOOK_H
#define JNI_HOOK_H

#include <jni.h>
#include <string>
#include <unordered_map>
#include <mutex>

namespace BlackBox {

class JniHook {
public:
    static bool HookJniFun(JNIEnv* env, jobject method, void* newFunc, void** oldFunc, bool enable);
    static bool HookJniFun(JNIEnv* env, const char* className, const char* methodName, 
                          const char* signature, void* newFunc, void** oldFunc, bool enable);
    static void InitJniHook(JNIEnv* env, int sdkVersion);
    
    struct HookInfo {
        void* originalFunc;
        void* newFunc;
        bool enabled;
        std::string className;
        std::string methodName;
        std::string signature;
    };
    
private:
    static std::unordered_map<void*, HookInfo> hookMap_;
    static std::mutex hookMutex_;
    static bool initialized_;
    static int sdkVersion_;
    
    static jmethodID findMethod(JNIEnv* env, const char* className, 
                               const char* methodName, const char* signature);
    static void* getMethodAddress(JNIEnv* env, jmethodID method);
    static bool hookNativeMethod(void* methodAddr, void* newFunc, void** oldFunc);
};

}

#endif // JNI_HOOK_H

#include "BlackBoxCore.h"
#include "Utils.h"
#include "Obfuscator.h"
#include <android/log.h>
#include <unistd.h>
#include <sys/system_properties.h>

#define TAG "BlackBox"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace BlackBox {

JavaVM* BoxCore::javaVM = nullptr;
int BoxCore::apiLevel = 0;
std::mutex BoxCore::mutex_;

int BoxCore::getApiLevel() {
    if (apiLevel == 0) {
        char sdk_ver_str[PROP_VALUE_MAX] = {0};
        __system_property_get("ro.build.version.sdk", sdk_ver_str);
        apiLevel = atoi(sdk_ver_str);
        if (apiLevel == 0) {
            apiLevel = 21;
        }
    }
    return apiLevel;
}

void BoxCore::loadEmptyDex(JNIEnv* env) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOGD("loadEmptyDex: Creating empty DEX file");
    
    jclass dexClassLoaderClass = env->FindClass("dalvik/system/DexClassLoader");
    if (dexClassLoaderClass == nullptr) {
        LOGE("Failed to find DexClassLoader class");
        return;
    }
    
    const char* emptyDexPath = "/data/local/tmp/empty.dex";
    jstring dexPath = env->NewStringUTF(emptyDexPath);
    jstring optimizedDir = env->NewStringUTF("/data/local/tmp");
    
    jmethodID constructor = env->GetMethodID(dexClassLoaderClass, "<init>", 
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
    
    if (constructor) {
        jobject dexClassLoader = env->NewObject(dexClassLoaderClass, constructor, 
            dexPath, optimizedDir, nullptr, nullptr);
        if (dexClassLoader) {
            LOGD("Empty DEX loaded successfully");
            env->DeleteLocalRef(dexClassLoader);
        }
    }
    
    env->DeleteLocalRef(dexPath);
    env->DeleteLocalRef(optimizedDir);
    env->DeleteLocalRef(dexClassLoaderClass);
}

jint BoxCore::getCallingUid(JNIEnv* env, int uid) {
    if (uid < 0) {
        return getuid();
    }
    
    jclass binderClass = env->FindClass("android/os/Binder");
    if (binderClass) {
        jmethodID getCallingUid = env->GetStaticMethodID(binderClass, 
            "getCallingUid", "()I");
        if (getCallingUid) {
            jint callingUid = env->CallStaticIntMethod(binderClass, getCallingUid);
            env->DeleteLocalRef(binderClass);
            return callingUid;
        }
        env->DeleteLocalRef(binderClass);
    }
    
    return uid;
}

jobject BoxCore::redirectPathFile(JNIEnv* env, jobject file) {
    if (file == nullptr) {
        return nullptr;
    }
    
    jclass fileClass = env->GetObjectClass(file);
    jmethodID getPathMethod = env->GetMethodID(fileClass, "getPath", 
        "()Ljava/lang/String;");
    
    if (getPathMethod) {
        jstring path = (jstring)env->CallObjectMethod(file, getPathMethod);
        if (path) {
            jstring redirectedPath = redirectPathString(env, path);
            if (redirectedPath != path) {
                jmethodID fileConstructor = env->GetMethodID(fileClass, "<init>", 
                    "(Ljava/lang/String;)V");
                if (fileConstructor) {
                    jobject newFile = env->NewObject(fileClass, fileConstructor, 
                        redirectedPath);
                    env->DeleteLocalRef(path);
                    env->DeleteLocalRef(redirectedPath);
                    env->DeleteLocalRef(fileClass);
                    return newFile;
                }
            }
            env->DeleteLocalRef(path);
            env->DeleteLocalRef(redirectedPath);
        }
    }
    
    env->DeleteLocalRef(fileClass);
    return file;
}

jstring BoxCore::redirectPathString(JNIEnv* env, jstring path) {
    if (path == nullptr) {
        return nullptr;
    }
    
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    if (pathStr) {
        std::string redirected = IO::redirectPath(pathStr);
        env->ReleaseStringUTFChars(path, pathStr);
        
        if (!redirected.empty()) {
            return env->NewStringUTF(redirected.c_str());
        }
    }
    
    return path;
}

JavaVM* BoxCore::getJavaVM() {
    return javaVM;
}

void BoxCore::setJavaVM(JavaVM* vm) {
    javaVM = vm;
}

void BoxCore::setApiLevel(int level) {
    apiLevel = level;
}

}

extern "C" {

jstring Java_top_niunaijun_blackbox_BlackBoxCore_HiddenUrl(JNIEnv* env, jclass clazz) {
    auto obfuscated = OBFUSCATE("https://github.com/niunaijun/BlackBox");
    return env->NewStringUTF(obfuscated.decrypt());
}

}

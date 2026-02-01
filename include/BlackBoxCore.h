#ifndef BLACKBOX_CORE_H
#define BLACKBOX_CORE_H

#include <jni.h>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <memory>

namespace BlackBox {

class BoxCore {
public:
    static int getApiLevel();
    static void loadEmptyDex(JNIEnv* env);
    static jint getCallingUid(JNIEnv* env, int uid);
    static jobject redirectPathFile(JNIEnv* env, jobject file);
    static jstring redirectPathString(JNIEnv* env, jstring path);
    static JavaVM* getJavaVM();
    
    static void setJavaVM(JavaVM* vm);
    static void setApiLevel(int level);
    
private:
    static JavaVM* javaVM;
    static int apiLevel;
    static std::mutex mutex_;
};

extern "C" {
    jstring Java_top_niunaijun_blackbox_BlackBoxCore_HiddenUrl(JNIEnv* env, jclass clazz);
}

}

#endif // BLACKBOX_CORE_H

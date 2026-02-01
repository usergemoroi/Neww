#ifndef HOOKS_H
#define HOOKS_H

#include <jni.h>

namespace BlackBox {

class BaseHook {
public:
    virtual void init(JNIEnv* env);
    virtual ~BaseHook() = default;
    
protected:
    bool initialized_;
    JNIEnv* env_;
};

class BinderHook : public BaseHook {
public:
    void init(JNIEnv* env) override;
    
private:
    static jclass binderClass_;
    static jmethodID getCallingUidMethod_;
    
    static jint hook_getCallingUid(JNIEnv* env, jobject thiz);
    static jint original_getCallingUid(JNIEnv* env, jobject thiz);
};

class RuntimeHook : public BaseHook {
public:
    void init(JNIEnv* env) override;
    
private:
    static jclass runtimeClass_;
    static jmethodID loadMethod_;
    static jmethodID execMethod_;
    
    static void hook_load(JNIEnv* env, jobject thiz, jstring library);
    static void original_load(JNIEnv* env, jobject thiz, jstring library);
    
    static jobject hook_exec(JNIEnv* env, jobject thiz, jobjectArray cmdarray);
    static jobject original_exec(JNIEnv* env, jobject thiz, jobjectArray cmdarray);
};

class UnixFileSystemHook : public BaseHook {
public:
    void init(JNIEnv* env) override;
    
private:
    static jclass fileSystemClass_;
    static jmethodID canonicalizeMethod_;
    static jmethodID listMethod_;
    static jmethodID getBooleanAttributesMethod_;
    
    static jstring hook_canonicalize(JNIEnv* env, jobject thiz, jstring path);
    static jstring original_canonicalize(JNIEnv* env, jobject thiz, jstring path);
    
    static jobjectArray hook_list(JNIEnv* env, jobject thiz, jobject file);
    static jobjectArray original_list(JNIEnv* env, jobject thiz, jobject file);
    
    static jint hook_getBooleanAttributes(JNIEnv* env, jobject thiz, jstring path);
    static jint original_getBooleanAttributes(JNIEnv* env, jobject thiz, jstring path);
};

class VMClassLoaderHook : public BaseHook {
public:
    void init(JNIEnv* env) override;
    static void hideXposed();
    
private:
    static jclass classLoaderClass_;
    static jmethodID findLoadedClassMethod_;
    static jmethodID loadClassMethod_;
    
    static jclass hook_findLoadedClass(JNIEnv* env, jobject thiz, jobject classLoader, jstring name);
    static jclass original_findLoadedClass(JNIEnv* env, jobject thiz, jobject classLoader, jstring name);
    
    static jclass hook_loadClass(JNIEnv* env, jobject thiz, jstring name);
    static jclass original_loadClass(JNIEnv* env, jobject thiz, jstring name);
    
    static bool isXposedClass(const char* className);
    static bool shouldHideClass(const char* className);
};

class PointerCheck {
public:
    static bool check(void* ptr);
    
private:
    static bool isValidMemoryAddress(void* ptr);
    static bool isInValidRange(void* ptr);
};

}

#endif // HOOKS_H

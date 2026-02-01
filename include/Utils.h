#ifndef UTILS_H
#define UTILS_H

#include <jni.h>
#include <string>
#include <vector>

namespace BlackBox {

extern "C" JavaVM* VMEnv;

JNIEnv* getEnv();
void ensureEnvCreated();
bool file_exists(const char* path);
std::string replace(const char* str, const char* from, const char* to);
void HexDump(char* data, int len, int width);

bool CheckFlags(void* flags);

void* fake_dlopen(const char* filename, int flag);
void* fake_dlsym(void* handle, const char* symbol);
int fake_dlclose(void* handle);

jint init(JNIEnv* env, jobject thiz, int sdkVersion);
void nativeHook(JNIEnv* env);
void hideXposed(JNIEnv* env, jclass clazz);

void registerMethod(JNIEnv* env);
void registerNative(JNIEnv* env);
void registerNatives(JNIEnv* env);
jint registerNativeMethods(JNIEnv* env, const char* className, 
                          JNINativeMethod* methods, int numMethods);

bool checkRequiredMethods(JNIEnv* env);
void checkDebugger();

void enableIO(JNIEnv* env, jclass clazz);
void addIORule(JNIEnv* env, jclass clazz, jstring source, jstring target);

jstring new_canonicalize0(JNIEnv* env, jobject thiz, jstring path);
jobjectArray new_list0(JNIEnv* env, jobject thiz, jobject file);
jint new_getBooleanAttributes0(JNIEnv* env, jobject thiz, jstring path);
jboolean new_createFileExclusively0(JNIEnv* env, jobject thiz, jstring path);
jboolean new_createDirectory0(JNIEnv* env, jobject thiz, jobject file);
jlong new_getLastModifiedTime0(JNIEnv* env, jobject thiz, jobject file);
jboolean new_setLastModifiedTime0(JNIEnv* env, jobject thiz, jobject file, jobject time);
jboolean new_setReadOnly0(JNIEnv* env, jobject thiz, jobject file);
jboolean new_setPermission0(JNIEnv* env, jobject thiz, jobject file, int access, 
                            jboolean enable, jboolean ownerOnly);
jlong new_getSpace0(JNIEnv* env, jobject thiz, jobject file, int type);

void new_nativeLoad(JNIEnv* env, jobject thiz, jstring filename, jobject classLoader);
void new_nativeLoad2(JNIEnv* env, jobject thiz, jstring filename, jobject classLoader, jobject caller);

jclass new_findLoadedClass(JNIEnv* env, jobject thiz, jobject classLoader, jstring name);

jint new_getCallingUid(JNIEnv* env, jobject thiz);

jlong native_offset(JNIEnv* env, jclass clazz);
jlong native_offset2(JNIEnv* env, jclass clazz);

void set_method_accessible(JNIEnv* env, jclass clazz, jclass target, jobject method);
void set_field_accessible(JNIEnv* env, jclass clazz, jclass target, jobject field);

namespace _lxy_oxor_any_ {
    void X();
    void Y();
}

}

#endif // UTILS_H

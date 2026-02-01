#ifndef IO_H
#define IO_H

#include <jni.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

namespace BlackBox {

class IO {
public:
    struct RelocateInfo {
        std::string sourcePath;
        std::string targetPath;
        bool enabled;
        
        RelocateInfo(const std::string& src, const std::string& tgt) 
            : sourcePath(src), targetPath(tgt), enabled(true) {}
    };
    
    static std::string redirectPath(const char* path);
    static jstring redirectPath(JNIEnv* env, jstring path);
    static jobject redirectPath(JNIEnv* env, jobject file);
    static void addRule(const char* source, const char* target);
    static void init(JNIEnv* env);
    
    static void clearRules();
    static const std::vector<RelocateInfo>& getRules();
    
private:
    static std::vector<RelocateInfo> relocateRules_;
    static std::mutex ioMutex_;
    static bool initialized_;
    
    static bool matchPath(const std::string& path, const std::string& pattern);
    static std::string applyRule(const std::string& path, const RelocateInfo& rule);
    static std::string normalizePath(const std::string& path);
};

}

#endif // IO_H

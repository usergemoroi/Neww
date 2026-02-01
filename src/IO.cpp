#include "IO.h"
#include "BlackBoxCore.h"
#include <android/log.h>
#include <algorithm>
#include <cstring>

#define TAG "BlackBoxIO"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace BlackBox {

std::vector<IO::RelocateInfo> IO::relocateRules_;
std::mutex IO::ioMutex_;
bool IO::initialized_ = false;

void IO::init(JNIEnv* env) {
    std::lock_guard<std::mutex> lock(ioMutex_);
    
    if (initialized_) {
        LOGD("IO already initialized");
        return;
    }
    
    initialized_ = true;
    LOGD("IO system initialized");
}

void IO::addRule(const char* source, const char* target) {
    if (!source || !target) {
        LOGE("Invalid rule parameters");
        return;
    }
    
    std::lock_guard<std::mutex> lock(ioMutex_);
    
    std::string srcPath = normalizePath(source);
    std::string tgtPath = normalizePath(target);
    
    for (auto& rule : relocateRules_) {
        if (rule.sourcePath == srcPath) {
            rule.targetPath = tgtPath;
            rule.enabled = true;
            LOGD("Updated rule: %s -> %s", srcPath.c_str(), tgtPath.c_str());
            return;
        }
    }
    
    relocateRules_.emplace_back(srcPath, tgtPath);
    LOGD("Added new rule: %s -> %s", srcPath.c_str(), tgtPath.c_str());
}

std::string IO::redirectPath(const char* path) {
    if (!path || !initialized_) {
        return path ? path : "";
    }
    
    std::lock_guard<std::mutex> lock(ioMutex_);
    
    std::string normalizedPath = normalizePath(path);
    
    for (const auto& rule : relocateRules_) {
        if (!rule.enabled) {
            continue;
        }
        
        if (matchPath(normalizedPath, rule.sourcePath)) {
            std::string result = applyRule(normalizedPath, rule);
            if (!result.empty() && result != normalizedPath) {
                LOGD("Redirected: %s -> %s", path, result.c_str());
                return result;
            }
        }
    }
    
    return path;
}

jstring IO::redirectPath(JNIEnv* env, jstring path) {
    if (!path || !env) {
        return path;
    }
    
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    if (!pathStr) {
        return path;
    }
    
    std::string redirected = redirectPath(pathStr);
    env->ReleaseStringUTFChars(path, pathStr);
    
    if (redirected.empty() || redirected == pathStr) {
        return path;
    }
    
    return env->NewStringUTF(redirected.c_str());
}

jobject IO::redirectPath(JNIEnv* env, jobject file) {
    if (!file || !env) {
        return file;
    }
    
    jclass fileClass = env->GetObjectClass(file);
    if (!fileClass) {
        return file;
    }
    
    jmethodID getPathMethod = env->GetMethodID(fileClass, "getPath", 
        "()Ljava/lang/String;");
    if (!getPathMethod) {
        env->DeleteLocalRef(fileClass);
        return file;
    }
    
    jstring path = (jstring)env->CallObjectMethod(file, getPathMethod);
    if (!path) {
        env->DeleteLocalRef(fileClass);
        return file;
    }
    
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    if (!pathStr) {
        env->DeleteLocalRef(path);
        env->DeleteLocalRef(fileClass);
        return file;
    }
    
    std::string redirected = redirectPath(pathStr);
    env->ReleaseStringUTFChars(path, pathStr);
    
    if (redirected.empty() || redirected == pathStr) {
        env->DeleteLocalRef(path);
        env->DeleteLocalRef(fileClass);
        return file;
    }
    
    jstring redirectedPath = env->NewStringUTF(redirected.c_str());
    jmethodID constructor = env->GetMethodID(fileClass, "<init>", 
        "(Ljava/lang/String;)V");
    
    if (constructor) {
        jobject newFile = env->NewObject(fileClass, constructor, redirectedPath);
        env->DeleteLocalRef(redirectedPath);
        env->DeleteLocalRef(path);
        env->DeleteLocalRef(fileClass);
        return newFile;
    }
    
    env->DeleteLocalRef(redirectedPath);
    env->DeleteLocalRef(path);
    env->DeleteLocalRef(fileClass);
    return file;
}

void IO::clearRules() {
    std::lock_guard<std::mutex> lock(ioMutex_);
    relocateRules_.clear();
    LOGD("All IO rules cleared");
}

const std::vector<IO::RelocateInfo>& IO::getRules() {
    return relocateRules_;
}

bool IO::matchPath(const std::string& path, const std::string& pattern) {
    if (pattern.empty()) {
        return false;
    }
    
    if (pattern.back() == '*') {
        std::string prefix = pattern.substr(0, pattern.length() - 1);
        return path.compare(0, prefix.length(), prefix) == 0;
    }
    
    if (pattern.back() == '/') {
        return path.compare(0, pattern.length(), pattern) == 0 ||
               path == pattern.substr(0, pattern.length() - 1);
    }
    
    return path == pattern;
}

std::string IO::applyRule(const std::string& path, const RelocateInfo& rule) {
    if (rule.sourcePath.back() == '*') {
        std::string prefix = rule.sourcePath.substr(0, rule.sourcePath.length() - 1);
        if (path.compare(0, prefix.length(), prefix) == 0) {
            return rule.targetPath + path.substr(prefix.length());
        }
    } else if (rule.sourcePath.back() == '/') {
        std::string dirPath = rule.sourcePath.substr(0, rule.sourcePath.length() - 1);
        if (path.compare(0, rule.sourcePath.length(), rule.sourcePath) == 0) {
            return rule.targetPath + "/" + path.substr(rule.sourcePath.length());
        } else if (path == dirPath) {
            return rule.targetPath;
        }
    } else {
        if (path == rule.sourcePath) {
            return rule.targetPath;
        }
    }
    
    return path;
}

std::string IO::normalizePath(const std::string& path) {
    if (path.empty()) {
        return path;
    }
    
    std::string result = path;
    
    while (result.length() > 1 && result.back() == '/') {
        result.pop_back();
    }
    
    size_t pos = 0;
    while ((pos = result.find("//", pos)) != std::string::npos) {
        result.erase(pos, 1);
    }
    
    return result;
}

}

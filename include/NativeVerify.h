#ifndef NATIVE_VERIFY_H
#define NATIVE_VERIFY_H

#include <jni.h>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace BlackBox {

class NativeVerify {
public:
    static bool checkHostReachable(const char* host, int port);
    static jboolean verifyUrlAndReturn(JNIEnv* env);
    static void* verification_worker(void* arg);
    
    static void startVerification();
    static void stopVerification();
    static bool isVerified();
    
    struct VerificationContext {
        std::string host;
        int port;
        int timeout;
        bool completed;
        bool result;
    };
    
private:
    static std::atomic<bool> verificationRunning_;
    static std::atomic<bool> verified_;
    static std::thread verificationThread_;
    static std::mutex verifyMutex_;
    static std::condition_variable verifyCv_;
    
    static int connectWithTimeout(const char* host, int port, int timeout);
    static bool performHttpCheck(const char* host, int port);
};

extern "C" {
    jboolean verifyUrlAndReturn(JNIEnv* env);
}

}

#endif // NATIVE_VERIFY_H

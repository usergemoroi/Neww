#include "NativeVerify.h"
#include "Obfuscator.h"
#include <android/log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <chrono>

#define TAG "NativeVerify"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace BlackBox {

std::atomic<bool> NativeVerify::verificationRunning_(false);
std::atomic<bool> NativeVerify::verified_(false);
std::thread NativeVerify::verificationThread_;
std::mutex NativeVerify::verifyMutex_;
std::condition_variable NativeVerify::verifyCv_;

bool NativeVerify::checkHostReachable(const char* host, int port) {
    if (!host || port <= 0 || port > 65535) {
        LOGE("Invalid host or port");
        return false;
    }
    
    LOGD("Checking host reachability: %s:%d", host, port);
    
    int result = connectWithTimeout(host, port, 5000);
    
    if (result == 0) {
        LOGD("Host %s:%d is reachable", host, port);
        return true;
    } else {
        LOGD("Host %s:%d is not reachable", host, port);
        return false;
    }
}

jboolean NativeVerify::verifyUrlAndReturn(JNIEnv* env) {
    if (verified_.load()) {
        return JNI_TRUE;
    }
    
    auto hostObf = OBFUSCATE_KEY(122, "api.example.com");
    const char* host = hostObf.decrypt();
    int port = 443;
    
    bool result = checkHostReachable(host, port);
    
    if (result) {
        verified_.store(true);
        return JNI_TRUE;
    }
    
    return JNI_FALSE;
}

void* NativeVerify::verification_worker(void* arg) {
    VerificationContext* ctx = static_cast<VerificationContext*>(arg);
    
    if (!ctx) {
        return nullptr;
    }
    
    LOGD("Verification worker started for %s:%d", ctx->host.c_str(), ctx->port);
    
    ctx->result = checkHostReachable(ctx->host.c_str(), ctx->port);
    ctx->completed = true;
    
    LOGD("Verification worker completed with result: %d", ctx->result);
    
    return nullptr;
}

void NativeVerify::startVerification() {
    if (verificationRunning_.load()) {
        LOGD("Verification already running");
        return;
    }
    
    verificationRunning_.store(true);
    
    verificationThread_ = std::thread([]() {
        VerificationContext ctx;
        ctx.host = "api.example.com";
        ctx.port = 443;
        ctx.timeout = 10000;
        ctx.completed = false;
        ctx.result = false;
        
        verification_worker(&ctx);
        
        if (ctx.result) {
            verified_.store(true);
        }
        
        verificationRunning_.store(false);
    });
    
    verificationThread_.detach();
}

void NativeVerify::stopVerification() {
    verificationRunning_.store(false);
}

bool NativeVerify::isVerified() {
    return verified_.load();
}

int NativeVerify::connectWithTimeout(const char* host, int port, int timeout) {
    struct addrinfo hints, *servinfo, *p;
    int sockfd;
    int rv;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    std::string portStr = std::to_string(port);
    
    if ((rv = getaddrinfo(host, portStr.c_str(), &hints, &servinfo)) != 0) {
        LOGE("getaddrinfo: %s", gai_strerror(rv));
        return -1;
    }
    
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        
        rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
        
        if (rv == -1 && errno != EINPROGRESS) {
            close(sockfd);
            continue;
        }
        
        if (rv == 0) {
            freeaddrinfo(servinfo);
            close(sockfd);
            return 0;
        }
        
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(sockfd, &writefds);
        
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        
        rv = select(sockfd + 1, nullptr, &writefds, nullptr, &tv);
        
        if (rv > 0) {
            int error = 0;
            socklen_t len = sizeof(error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
            
            if (error == 0) {
                close(sockfd);
                freeaddrinfo(servinfo);
                return 0;
            }
        }
        
        close(sockfd);
    }
    
    freeaddrinfo(servinfo);
    return -1;
}

bool NativeVerify::performHttpCheck(const char* host, int port) {
    int result = connectWithTimeout(host, port, 5000);
    return result == 0;
}

}

extern "C" {

jboolean verifyUrlAndReturn(JNIEnv* env) {
    return BlackBox::NativeVerify::verifyUrlAndReturn(env);
}

}

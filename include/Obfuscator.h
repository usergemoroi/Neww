#ifndef OBFUSCATOR_H
#define OBFUSCATOR_H

#include <cstddef>
#include <array>
#include <string>

namespace ay {

template<std::size_t N, char KEY>
class obfuscator {
public:
    constexpr obfuscator(const char* data) : data_() {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] = data[i] ^ KEY;
        }
    }
    
    constexpr char operator[](std::size_t i) const {
        return data_[i];
    }
    
    constexpr std::size_t size() const {
        return N;
    }
    
    constexpr char key() const {
        return KEY;
    }
    
private:
    char data_[N];
};

template<std::size_t N, char KEY>
class OBFUSCATE_data {
public:
    OBFUSCATE_data(const obfuscator<N, KEY>& obf) : obfuscator_(obf), decrypted_(false) {}
    
    ~OBFUSCATE_data() {
        if (decrypted_) {
            for (std::size_t i = 0; i < decryptedData_.size(); ++i) {
                decryptedData_[i] = 0;
            }
        }
    }
    
    const char* decrypt() {
        if (!decrypted_) {
            for (std::size_t i = 0; i < N; ++i) {
                decryptedData_[i] = obfuscator_[i] ^ KEY;
            }
            decryptedData_[N] = '\0';
            decrypted_ = true;
        }
        return decryptedData_.data();
    }
    
    operator const char*() {
        return decrypt();
    }
    
    std::string str() {
        return std::string(decrypt());
    }
    
private:
    const obfuscator<N, KEY>& obfuscator_;
    std::array<char, N + 1> decryptedData_;
    bool decrypted_;
};

template<std::size_t N>
constexpr std::size_t get_size(const char(&)[N]) {
    return N - 1;
}

#define OBFUSCATE_KEY(key, data) \
    []() { \
        constexpr auto size = ay::get_size(data); \
        constexpr ay::obfuscator<size, key> obfuscator(data); \
        return ay::OBFUSCATE_data<size, key>(obfuscator); \
    }()

#define OBFUSCATE(data) OBFUSCATE_KEY(0xAB, data)

}

#endif // OBFUSCATOR_H

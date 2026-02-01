#include "Obfuscator.h"
#include <cassert>
#include <iostream>
#include <cstring>

using namespace ay;

void test_basic_obfuscation() {
    std::cout << "Testing basic obfuscation..." << std::endl;
    
    auto obf = OBFUSCATE("Hello, World!");
    const char* decrypted = obf.decrypt();
    
    assert(strcmp(decrypted, "Hello, World!") == 0);
    
    std::cout << "  ✓ Basic obfuscation passed" << std::endl;
}

void test_custom_key() {
    std::cout << "Testing custom key obfuscation..." << std::endl;
    
    auto obf = OBFUSCATE_KEY(0x42, "Secret Key");
    const char* decrypted = obf.decrypt();
    
    assert(strcmp(decrypted, "Secret Key") == 0);
    
    std::cout << "  ✓ Custom key obfuscation passed" << std::endl;
}

void test_empty_string() {
    std::cout << "Testing empty string obfuscation..." << std::endl;
    
    auto obf = OBFUSCATE("");
    const char* decrypted = obf.decrypt();
    
    assert(strcmp(decrypted, "") == 0);
    
    std::cout << "  ✓ Empty string obfuscation passed" << std::endl;
}

void test_string_conversion() {
    std::cout << "Testing string conversion..." << std::endl;
    
    auto obf = OBFUSCATE("Test String");
    std::string str = obf.str();
    
    assert(str == "Test String");
    
    std::cout << "  ✓ String conversion passed" << std::endl;
}

void test_multiple_decrypt_calls() {
    std::cout << "Testing multiple decrypt calls..." << std::endl;
    
    auto obf = OBFUSCATE("Multiple Calls");
    const char* first = obf.decrypt();
    const char* second = obf.decrypt();
    
    assert(strcmp(first, "Multiple Calls") == 0);
    assert(strcmp(second, "Multiple Calls") == 0);
    assert(first == second);
    
    std::cout << "  ✓ Multiple decrypt calls passed" << std::endl;
}

void test_long_string() {
    std::cout << "Testing long string obfuscation..." << std::endl;
    
    auto obf = OBFUSCATE("This is a very long string that should be properly obfuscated and decrypted without any issues");
    const char* decrypted = obf.decrypt();
    
    assert(strcmp(decrypted, "This is a very long string that should be properly obfuscated and decrypted without any issues") == 0);
    
    std::cout << "  ✓ Long string obfuscation passed" << std::endl;
}

int main() {
    std::cout << "Running Obfuscator tests..." << std::endl << std::endl;
    
    try {
        test_basic_obfuscation();
        test_custom_key();
        test_empty_string();
        test_string_conversion();
        test_multiple_decrypt_calls();
        test_long_string();
        
        std::cout << std::endl << "All Obfuscator tests passed! ✓" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}

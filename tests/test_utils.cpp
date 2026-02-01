#include "Utils.h"
#include <cassert>
#include <iostream>
#include <string>
#include <cstring>

using namespace BlackBox;

void test_string_replace() {
    std::cout << "Testing string replace..." << std::endl;
    
    std::string result = replace("Hello World", "World", "Universe");
    assert(result == "Hello Universe");
    
    result = replace("test test test", "test", "pass");
    assert(result == "pass pass pass");
    
    result = replace("no match", "xyz", "abc");
    assert(result == "no match");
    
    std::cout << "  ✓ String replace passed" << std::endl;
}

void test_file_exists() {
    std::cout << "Testing file existence check..." << std::endl;
    
    assert(!file_exists(nullptr));
    assert(!file_exists(""));
    assert(!file_exists("/nonexistent/path/file.txt"));
    
    std::cout << "  ✓ File existence check passed" << std::endl;
}

void test_pointer_check() {
    std::cout << "Testing pointer validation..." << std::endl;
    
    assert(!PointerCheck::check(nullptr));
    assert(!PointerCheck::check((void*)0x100));
    
    int validData = 42;
    assert(PointerCheck::check(&validData));
    
    std::cout << "  ✓ Pointer validation passed" << std::endl;
}

void test_check_flags() {
    std::cout << "Testing flags check..." << std::endl;
    
    assert(!CheckFlags(nullptr));
    
    int flags = 0;
    assert(CheckFlags(&flags));
    
    std::cout << "  ✓ Flags check passed" << std::endl;
}

int main() {
    std::cout << "Running Utils tests..." << std::endl << std::endl;
    
    try {
        test_string_replace();
        test_file_exists();
        test_pointer_check();
        test_check_flags();
        
        std::cout << std::endl << "All Utils tests passed! ✓" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}

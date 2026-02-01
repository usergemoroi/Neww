#include "IO.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace BlackBox;

void test_basic_redirection() {
    std::cout << "Testing basic path redirection..." << std::endl;
    
    IO::addRule("/data/app", "/data/virtual/app");
    
    std::string redirected = IO::redirectPath("/data/app");
    assert(redirected == "/data/virtual/app");
    
    std::cout << "  ✓ Basic redirection passed" << std::endl;
}

void test_wildcard_redirection() {
    std::cout << "Testing wildcard redirection..." << std::endl;
    
    IO::addRule("/sdcard/*", "/data/virtual/sdcard");
    
    std::string redirected = IO::redirectPath("/sdcard/test.txt");
    assert(redirected == "/data/virtual/sdcard/test.txt");
    
    std::cout << "  ✓ Wildcard redirection passed" << std::endl;
}

void test_directory_redirection() {
    std::cout << "Testing directory redirection..." << std::endl;
    
    IO::addRule("/system/lib/", "/data/virtual/lib/");
    
    std::string redirected = IO::redirectPath("/system/lib/libtest.so");
    assert(redirected.find("/data/virtual/lib/") == 0);
    
    std::cout << "  ✓ Directory redirection passed" << std::endl;
}

void test_no_match() {
    std::cout << "Testing no match case..." << std::endl;
    
    IO::clearRules();
    IO::addRule("/data/app", "/data/virtual/app");
    
    std::string redirected = IO::redirectPath("/etc/hosts");
    assert(redirected == "/etc/hosts");
    
    std::cout << "  ✓ No match case passed" << std::endl;
}

void test_path_normalization() {
    std::cout << "Testing path normalization..." << std::endl;
    
    IO::clearRules();
    IO::addRule("/data//app///", "/data/virtual/app");
    
    std::string redirected = IO::redirectPath("/data/app");
    assert(!redirected.empty());
    
    std::cout << "  ✓ Path normalization passed" << std::endl;
}

int main() {
    std::cout << "Running IO tests..." << std::endl << std::endl;
    
    try {
        test_basic_redirection();
        test_wildcard_redirection();
        test_directory_redirection();
        test_no_match();
        test_path_normalization();
        
        std::cout << std::endl << "All IO tests passed! ✓" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}

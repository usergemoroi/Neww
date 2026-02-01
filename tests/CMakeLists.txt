cmake_minimum_required(VERSION 3.10)

project(BlackBoxTests)

enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(test_io test_io.cpp)
target_link_libraries(test_io BlackBox)
add_test(NAME IORedirectionTest COMMAND test_io)

add_executable(test_obfuscator test_obfuscator.cpp)
target_link_libraries(test_obfuscator BlackBox)
add_test(NAME ObfuscatorTest COMMAND test_obfuscator)

add_executable(test_utils test_utils.cpp)
target_link_libraries(test_utils BlackBox)
add_test(NAME UtilsTest COMMAND test_utils)

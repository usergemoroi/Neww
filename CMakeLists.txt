cmake_minimum_required(VERSION 3.10)
project(BlackBox VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fvisibility=hidden")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

if(ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fstack-protector-strong")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCE_FILES
    src/BlackBoxCore.cpp
    src/JniHook.cpp
    src/IO.cpp
    src/NativeVerify.cpp
    src/Hooks.cpp
    src/Utils.cpp
)

set(HEADER_FILES
    include/BlackBoxCore.h
    include/JniHook.h
    include/IO.h
    include/NativeVerify.h
    include/Hooks.h
    include/Utils.h
    include/Obfuscator.h
)

add_library(BlackBox SHARED ${SOURCE_FILES} ${HEADER_FILES})

if(ANDROID)
    find_library(log-lib log)
    find_library(android-lib android)
    
    target_link_libraries(BlackBox
        ${log-lib}
        ${android-lib}
        dl
        m
        c
    )
else()
    target_link_libraries(BlackBox
        dl
        pthread
        m
        c
    )
endif()

set_target_properties(BlackBox PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "BlackBox"
    PREFIX "lib"
)

set_target_properties(BlackBox PROPERTIES
    PUBLIC_HEADER "${HEADER_FILES}"
)

install(TARGETS BlackBox
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/blackbox
)

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

add_custom_target(format
    COMMAND clang-format -i ${SOURCE_FILES} ${HEADER_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting source code"
)

add_custom_target(strip_release
    COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:BlackBox>
    DEPENDS BlackBox
    COMMENT "Stripping release binary"
)

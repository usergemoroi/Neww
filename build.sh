#!/bin/bash

set -e

# BlackBox Build Script
# Comprehensive build automation for all platforms and configurations

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
ANDROID_ABIS=("arm64-v8a" "armeabi-v7a" "x86_64" "x86")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print colored message
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Android NDK is available
check_ndk() {
    if [ -z "${ANDROID_NDK}" ]; then
        log_error "ANDROID_NDK environment variable not set"
        log_info "Please set ANDROID_NDK to your NDK installation path"
        log_info "Example: export ANDROID_NDK=/path/to/android-ndk-r21e"
        return 1
    fi
    
    if [ ! -d "${ANDROID_NDK}" ]; then
        log_error "ANDROID_NDK directory does not exist: ${ANDROID_NDK}"
        return 1
    fi
    
    log_info "Using Android NDK: ${ANDROID_NDK}"
    return 0
}

# Build for Android using CMake
build_android_cmake() {
    local abi=$1
    local build_type=${2:-Release}
    
    log_info "Building for Android ${abi} (${build_type})..."
    
    local build_subdir="${BUILD_DIR}/android_${abi}_${build_type}"
    mkdir -p "${build_subdir}"
    
    cd "${build_subdir}"
    
    cmake "${SCRIPT_DIR}" \
        -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK}/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="${abi}" \
        -DANDROID_PLATFORM=android-21 \
        -DCMAKE_BUILD_TYPE="${build_type}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DBUILD_TESTS=OFF
    
    make -j$(nproc)
    
    local output_dir="${SCRIPT_DIR}/output/android/${abi}"
    mkdir -p "${output_dir}"
    
    cp libBlackBox.so "${output_dir}/"
    
    if [ "${build_type}" == "Release" ]; then
        ${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip \
            --strip-all "${output_dir}/libBlackBox.so"
        log_info "Stripped ${output_dir}/libBlackBox.so"
    fi
    
    cd "${SCRIPT_DIR}"
    
    log_info "Build complete: ${output_dir}/libBlackBox.so"
}

# Build all Android architectures
build_android_all() {
    local build_type=${1:-Release}
    
    if ! check_ndk; then
        return 1
    fi
    
    log_info "Building for all Android architectures..."
    
    for abi in "${ANDROID_ABIS[@]}"; do
        build_android_cmake "${abi}" "${build_type}"
    done
    
    log_info "All Android builds complete!"
    log_info "Output directory: ${SCRIPT_DIR}/output/android/"
}

# Build for host system (Linux)
build_host() {
    local build_type=${1:-Release}
    
    log_info "Building for host system (${build_type})..."
    
    local build_subdir="${BUILD_DIR}/host_${build_type}"
    mkdir -p "${build_subdir}"
    
    cd "${build_subdir}"
    
    cmake "${SCRIPT_DIR}" \
        -DCMAKE_BUILD_TYPE="${build_type}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DBUILD_TESTS=ON
    
    make -j$(nproc)
    
    cd "${SCRIPT_DIR}"
    
    log_info "Host build complete: ${build_subdir}/libBlackBox.so"
}

# Run tests
run_tests() {
    log_info "Running tests..."
    
    local test_dir="${BUILD_DIR}/host_Debug"
    
    if [ ! -d "${test_dir}" ]; then
        log_warn "Test directory not found, building debug version first..."
        build_host Debug
    fi
    
    cd "${test_dir}"
    ctest --output-on-failure
    cd "${SCRIPT_DIR}"
    
    log_info "Tests complete!"
}

# Clean build artifacts
clean() {
    log_info "Cleaning build artifacts..."
    
    rm -rf "${BUILD_DIR}"
    rm -rf "${SCRIPT_DIR}/output"
    
    log_info "Clean complete!"
}

# Create distribution package
package() {
    local version=${1:-1.0.0}
    
    log_info "Creating distribution package..."
    
    local package_dir="${BUILD_DIR}/package/BlackBox-${version}"
    mkdir -p "${package_dir}"
    
    # Copy headers
    mkdir -p "${package_dir}/include"
    cp -r "${SCRIPT_DIR}/include/"*.h "${package_dir}/include/"
    
    # Copy libraries
    if [ -d "${SCRIPT_DIR}/output/android" ]; then
        mkdir -p "${package_dir}/lib/android"
        cp -r "${SCRIPT_DIR}/output/android/"* "${package_dir}/lib/android/"
    fi
    
    # Copy documentation
    mkdir -p "${package_dir}/docs"
    cp "${SCRIPT_DIR}/README.md" "${package_dir}/"
    cp "${SCRIPT_DIR}/docs/"*.md "${package_dir}/docs/" 2>/dev/null || true
    
    # Create archive
    cd "${BUILD_DIR}/package"
    tar -czf "BlackBox-${version}.tar.gz" "BlackBox-${version}"
    
    cd "${SCRIPT_DIR}"
    
    log_info "Package created: ${BUILD_DIR}/package/BlackBox-${version}.tar.gz"
}

# Analyze binary
analyze() {
    local binary=${1:-libBlackBox.so}
    
    if [ ! -f "${binary}" ]; then
        log_error "Binary not found: ${binary}"
        return 1
    fi
    
    log_info "Analyzing binary: ${binary}"
    
    echo ""
    echo "=== File Information ==="
    file "${binary}"
    
    echo ""
    echo "=== Size ==="
    ls -lh "${binary}" | awk '{print $5}'
    
    echo ""
    echo "=== ELF Header ==="
    readelf -h "${binary}" 2>/dev/null || echo "readelf not available"
    
    echo ""
    echo "=== Dynamic Symbols (first 20) ==="
    nm -D "${binary}" 2>/dev/null | head -20 || echo "nm not available"
    
    echo ""
    echo "=== Dependencies ==="
    readelf -d "${binary}" 2>/dev/null | grep NEEDED || echo "readelf not available"
}

# Show help
show_help() {
    cat << EOF
BlackBox Build Script

Usage: $0 [command] [options]

Commands:
    android [abi] [type]   Build for Android
                          abi: arm64-v8a, armeabi-v7a, x86_64, x86, all (default: all)
                          type: Release, Debug (default: Release)
    
    host [type]           Build for host system
                          type: Release, Debug (default: Release)
    
    test                  Run tests
    
    clean                 Clean build artifacts
    
    package [version]     Create distribution package
                          version: Version string (default: 1.0.0)
    
    analyze [binary]      Analyze binary file
                          binary: Path to .so file (default: libBlackBox.so)
    
    help                  Show this help message

Examples:
    $0 android all Release         # Build all Android ABIs in Release mode
    $0 android arm64-v8a Debug     # Build Android ARM64 in Debug mode
    $0 host Debug                  # Build for host in Debug mode
    $0 test                        # Run tests
    $0 package 1.0.0               # Create distribution package

Environment Variables:
    ANDROID_NDK           Path to Android NDK (required for Android builds)

EOF
}

# Main script logic
main() {
    local command=${1:-help}
    
    case "${command}" in
        android)
            local abi=${2:-all}
            local build_type=${3:-Release}
            
            if [ "${abi}" == "all" ]; then
                build_android_all "${build_type}"
            else
                if ! check_ndk; then
                    exit 1
                fi
                build_android_cmake "${abi}" "${build_type}"
            fi
            ;;
        
        host)
            local build_type=${2:-Release}
            build_host "${build_type}"
            ;;
        
        test)
            run_tests
            ;;
        
        clean)
            clean
            ;;
        
        package)
            local version=${2:-1.0.0}
            package "${version}"
            ;;
        
        analyze)
            local binary=${2:-libBlackBox.so}
            analyze "${binary}"
            ;;
        
        help|--help|-h)
            show_help
            ;;
        
        *)
            log_error "Unknown command: ${command}"
            echo ""
            show_help
            exit 1
            ;;
    esac
}

# Run main function
main "$@"

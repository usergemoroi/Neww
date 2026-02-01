# Makefile for BlackBox Native Library
# Alternative to CMake for simple builds

# Compiler and flags
CXX := clang++
CC := clang
CXXFLAGS := -std=c++17 -Wall -Wextra -fPIC -fvisibility=hidden
CFLAGS := -std=c11 -Wall -Wextra -fPIC
LDFLAGS := -shared -Wl,--exclude-libs,ALL

# Android NDK configuration
ifdef ANDROID_NDK
    TOOLCHAIN := $(ANDROID_NDK)/toolchains/llvm/prebuilt/linux-x86_64
    CXX := $(TOOLCHAIN)/bin/aarch64-linux-android21-clang++
    CC := $(TOOLCHAIN)/bin/aarch64-linux-android21-clang
    CXXFLAGS += -fstack-protector-strong
endif

# Build type
BUILD_TYPE ?= Release

ifeq ($(BUILD_TYPE),Debug)
    CXXFLAGS += -g -O0 -DDEBUG
    CFLAGS += -g -O0 -DDEBUG
else
    CXXFLAGS += -O3 -DNDEBUG
    CFLAGS += -O3 -DNDEBUG
endif

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib

# Source files
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Output
TARGET := $(LIB_DIR)/libBlackBox.so

# Include directories
INCLUDES := -I$(INC_DIR)

# Libraries
LIBS := -llog -landroid -ldl -lm -lc -lpthread

# Targets
.PHONY: all clean install strip test format

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Built $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build directory"

install: $(TARGET)
	@mkdir -p /usr/local/lib
	@mkdir -p /usr/local/include/blackbox
	cp $(TARGET) /usr/local/lib/
	cp $(INC_DIR)/*.h /usr/local/include/blackbox/
	@echo "Installed to /usr/local"

strip: $(TARGET)
	strip --strip-all $(TARGET)
	@echo "Stripped $(TARGET)"

# Android-specific targets
android-arm64:
	@$(MAKE) BUILD_TYPE=Release ANDROID_NDK=$(ANDROID_NDK)

android-arm:
	@$(MAKE) BUILD_TYPE=Release ANDROID_NDK=$(ANDROID_NDK) \
		CXX=$(TOOLCHAIN)/bin/armv7a-linux-androideabi21-clang++ \
		CC=$(TOOLCHAIN)/bin/armv7a-linux-androideabi21-clang

android-x86_64:
	@$(MAKE) BUILD_TYPE=Release ANDROID_NDK=$(ANDROID_NDK) \
		CXX=$(TOOLCHAIN)/bin/x86_64-linux-android21-clang++ \
		CC=$(TOOLCHAIN)/bin/x86_64-linux-android21-clang

android-x86:
	@$(MAKE) BUILD_TYPE=Release ANDROID_NDK=$(ANDROID_NDK) \
		CXX=$(TOOLCHAIN)/bin/i686-linux-android21-clang++ \
		CC=$(TOOLCHAIN)/bin/i686-linux-android21-clang

# Build all Android architectures
android-all: android-arm64 android-arm android-x86_64 android-x86

# Testing
test:
	@echo "Running tests..."
	@$(MAKE) -C tests all
	@$(MAKE) -C tests run

# Format code
format:
	clang-format -i $(SRC_DIR)/*.cpp $(INC_DIR)/*.h

# Show configuration
config:
	@echo "CXX: $(CXX)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "INCLUDES: $(INCLUDES)"
	@echo "LIBS: $(LIBS)"
	@echo "BUILD_TYPE: $(BUILD_TYPE)"

# Help
help:
	@echo "BlackBox Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build the library"
	@echo "  clean            - Remove build artifacts"
	@echo "  install          - Install to /usr/local"
	@echo "  strip            - Strip debug symbols"
	@echo "  android-arm64    - Build for Android ARM64"
	@echo "  android-arm      - Build for Android ARMv7"
	@echo "  android-x86_64   - Build for Android x86_64"
	@echo "  android-x86      - Build for Android x86"
	@echo "  android-all      - Build for all Android architectures"
	@echo "  test             - Run tests"
	@echo "  format           - Format source code"
	@echo "  config           - Show build configuration"
	@echo "  help             - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_TYPE       - Release or Debug (default: Release)"
	@echo "  ANDROID_NDK      - Path to Android NDK"

# Dependencies
-include $(OBJECTS:.o=.d)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MM -MT $(@:.d=.o) $< > $@

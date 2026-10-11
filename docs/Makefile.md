# Documentation: `Makefile`

The [`Makefile`](file:///home/rustam/Projects/resp-cpp/Makefile) defines the build configuration, compiler toolchain, optimization flags, sanitizers, and compilation targets for the RESP server.

---

### Line 1: `CXX = g++`
Defines the C++ compiler executable to be GNU C++ Compiler (`g++`).

### Line 2: `SRC_DIR = src`
Sets the relative path to the directory containing all C++ source files (`.cpp`).

### Line 3: `BUILD_DIR = build`
Sets the destination folder for compiled object files (`.o`) and final binary outputs.

### Line 4: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 5: `SOURCES = $(wildcard $(SRC_DIR)/*.cpp)`
Uses Make's built-in `wildcard` function to dynamically scan and collect all `.cpp` files in `src/` (`main.cpp`, `protocol.cpp`, `server.cpp`, `utils.cpp`).

### Line 6: `OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))`
Uses Make's `patsubst` (pattern substitution) function to transform each source path into an object file path inside the build directory (e.g., `src/main.cpp` $\to$ `build/main.o`).

### Line 7: `TARGET = $(BUILD_DIR)/resp-server`
Specifies the path and filename of the final compiled executable binary.

### Line 8: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 9: `# Default to maximum performance release build`
Make comment explaining compilation flag defaults.

### Line 10: `CXXFLAGS ?= -std=c++17 -Wall -Wextra -O3 -march=native -DNDEBUG`
Sets default compiler flags using conditional assignment (`?=`):
- `-std=c++17`: Enables C++17 language features (`std::string_view`, structured bindings, `constexpr`).
- `-Wall -Wextra`: Enables comprehensive compiler warning diagnostics.
- `-O3`: Activates highest optimization level (aggressive inlining, loop unrolling, vectorization).
- `-march=native`: Generates machine instructions specifically optimized for the host CPU architecture.
- `-DNDEBUG`: Disables assertions (`assert()`) for maximum execution speed.

### Line 11: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 12: `all: $(BUILD_DIR) $(TARGET)`
Default target. Ensures the `build/` directory exists, then builds the target binary.

### Line 13: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 14: `release:`
### Line 15: `	$(MAKE) clean`
### Line 16: `	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -O3 -march=native -DNDEBUG" all`
Release target that cleans previous artifacts and recompiles with maximum optimization flags.

### Line 17: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 18: `asan:`
### Line 19: `	$(MAKE) clean`
### Line 20: `	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -g -fsanitize=address,undefined" all`
Audit target that recompiles with AddressSanitizer and UndefinedBehaviorSanitizer for memory leak and runtime defect detection.

### Line 21: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 22: `debug:`
### Line 23: `	$(MAKE) clean`
### Line 24: `	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -g -DVERBOSE" all`
Debug target that recompiles with debugging symbols (`-g`) and verbose connection logging (`-DVERBOSE`).

### Line 25: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 26: `$(BUILD_DIR):`
### Line 27: `	mkdir -p $(BUILD_DIR)`
Rule creating the output build directory if it does not exist.

### Line 28: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 29: `$(TARGET): $(OBJECTS)`
### Line 30: `	$(CXX) $(CXXFLAGS) -o $@ $^`
Linking rule combining compiled object files (`$^`) into the final executable binary (`$@`).

### Line 31: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 32: `$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp`
### Line 33: `	$(CXX) $(CXXFLAGS) -c $< -o $@`
Pattern compilation rule transforming individual C++ source files (`$<`) into object files (`$@`).

### Line 34: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 35: `clean:`
### Line 36: `	rm -rf $(BUILD_DIR)`
Cleanup target removing the `build/` directory and all compiled binaries.

### Line 37: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 38: `.PHONY: all release asan debug clean`
Declares targets as phony to ensure execution regardless of matching files on the filesystem.

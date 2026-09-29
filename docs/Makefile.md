# Documentation: `Makefile`

The [`Makefile`](file:///home/rustam/Projects/resp-cpp/Makefile) defines the build configuration, compiler toolchain, optimization flags, sanitizers, and compilation targets for the RESP server.

---

## Complete Source Code

```makefile
1: CXX = g++
2: SRC_DIR = src
3: BUILD_DIR = build
4: 
5: SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
6: OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))
7: TARGET = $(BUILD_DIR)/resp-server
8: 
9: # Default to maximum performance release build
10: CXXFLAGS ?= -std=c++17 -Wall -Wextra -O3 -march=native -DNDEBUG
11: 
12: all: $(BUILD_DIR) $(TARGET)
13: 
14: release:
15: 	$(MAKE) clean
16: 	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -O3 -march=native -DNDEBUG" all
17: 
18: asan:
19: 	$(MAKE) clean
20: 	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -g -fsanitize=address,undefined" all
21: 
22: debug:
23: 	$(MAKE) clean
24: 	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -g -DVERBOSE" all
25: 
26: $(BUILD_DIR):
27: 	mkdir -p $(BUILD_DIR)
28: 
29: $(TARGET): $(OBJECTS)
30: 	$(CXX) $(CXXFLAGS) -o $@ $^
31: 
32: $(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
33: 	$(CXX) $(CXXFLAGS) -c $< -o $@
34: 
35: clean:
36: 	rm -rf $(BUILD_DIR)
37: 
38: .PHONY: all release asan debug clean
39: 
```

---

## Line-by-Line Breakdown & Explanation

### Variable Definitions (Lines 1–7)
- **Line 1: `CXX = g++`**  
  Defines the C++ compiler executable to be GNU C++ Compiler (`g++`).
- **Line 2: `SRC_DIR = src`**  
  Sets the relative path to the directory containing all C++ source files (`.cpp`).
- **Line 3: `BUILD_DIR = build`**  
  Sets the destination folder for compiled object files (`.o`) and final binary outputs.
- **Line 4: *(Empty Line)***  
  Visual formatting separation.
- **Line 5: `SOURCES = $(wildcard $(SRC_DIR)/*.cpp)`**  
  Uses Make's built-in `wildcard` function to dynamically scan and collect all `.cpp` files in `src/` (`main.cpp`, `protocol.cpp`, `server.cpp`, `utils.cpp`).
- **Line 6: `OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))`**  
  Uses Make's `patsubst` (pattern substitution) function to transform each source path into an object file path inside the build directory (e.g., `src/main.cpp` $\to$ `build/main.o`).
- **Line 7: `TARGET = $(BUILD_DIR)/resp-server`**  
  Specifies the path and filename of the final compiled executable binary.

---

### Default Compilation Flags (Lines 9–10)
- **Line 9: `# Default to maximum performance release build`**  
  Make comment explaining flag defaults.
- **Line 10: `CXXFLAGS ?= -std=c++17 -Wall -Wextra -O3 -march=native -DNDEBUG`**  
  Sets default compiler flags using conditional assignment (`?=`):
  - `-std=c++17`: Enables C++17 language features (`std::string_view`, structured bindings, `constexpr`).
  - `-Wall -Wextra`: Enables comprehensive compiler warning diagnostics.
  - `-O3`: Activates highest optimization level (aggressive inlining, loop unrolling, vectorization).
  - `-march=native`: Generates machine instructions specifically optimized for the host CPU architecture (AVX2, SSE4.2, etc.).
  - `-DNDEBUG`: Disables standard assertions (`assert()`) for maximum execution speed.

---

### Target Definitions (Lines 12–25)
- **Line 12: `all: $(BUILD_DIR) $(TARGET)`**  
  Default target. Ensures the `build/` directory exists, then builds the target binary.
- **Lines 14–16: `release:`**  
  Cleans build artifacts and re-compiles with maximum performance flags (`-O3 -march=native -DNDEBUG`).
- **Lines 18–20: `asan:`**  
  Cleans build artifacts and re-compiles with AddressSanitizer and UndefinedBehaviorSanitizer (`-fsanitize=address,undefined -O2 -g`). Detects memory leaks, use-after-free, and out-of-bounds array indexing at runtime.
- **Lines 22–24: `debug:`**  
  Cleans build artifacts and re-compiles with debugging symbols (`-g`) and connection logging enabled (`-DVERBOSE`).

---

### Build Rules (Lines 26–36)
- **Lines 26–27: `$(BUILD_DIR):`**  
  Rule to create the build directory: executes `mkdir -p build`.
- **Lines 29–30: `$(TARGET): $(OBJECTS)`**  
  Linking step: Links all compiled `.o` object files into the final executable `build/resp-server`.
  - `$@`: Automatic variable referring to the target name (`$(TARGET)`).
  - `$^`: Automatic variable referring to all prerequisites (`$(OBJECTS)`).
- **Lines 32–33: `$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp`**  
  Pattern rule to compile individual `.cpp` files into `.o` object files.
  - `-c`: Compile and assemble only, do not link.
  - `$<`: Automatic variable referring to the source file (first prerequisite).
  - `$@`: Automatic variable referring to the destination `.o` file.
- **Lines 35–36: `clean:`**  
  Cleanup rule: recursively deletes the `build/` directory.

---

### Phony Targets (Line 38)
- **Line 38: `.PHONY: all release asan debug clean`**  
  Declares that these targets do not correspond to actual files on the filesystem, ensuring they execute reliably even if a file with the target name exists.

CXX = g++
SRC_DIR = src
BUILD_DIR = build

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))
TARGET = $(BUILD_DIR)/resp-server

# Default to maximum performance release build
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O3 -march=native -DNDEBUG

all: $(BUILD_DIR) $(TARGET)

release:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -O3 -march=native -DNDEBUG" all

asan:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -g -fsanitize=address,undefined" all

debug:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="-std=c++17 -Wall -Wextra -g -DVERBOSE" all

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all release asan debug clean

TARGET = pdp

CXX = g++
CXX_FLAGS = -Wall -pedantic -Wextra -std=c++20

MKDIR = mkdir -p
BUILD_DIR = build

.PHONY: all
all: compile

.PHONY: compile
compile: $(TARGET)

.PHONY: run
run: $(TARGET)
	./$(TARGET)

$(TARGET): $(BUILD_DIR)/main.o
	$(CXX) $(CXX_FLAGS) $^ -o $@

$(BUILD_DIR)/%.o: src/%.cpp
	$(MKDIR) $(BUILD_DIR)
	$(CXX) $(CXX_FLAGS) $< -c -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET) $(BUILD_DIR)/ 2>/dev/null

#depencencies (generated by "g++ -MM src/*.cpp")
$(BUILD_DIR)main.o: src/main.cpp src/Graph.h src/Edge.h src/Vertex.h

#inspired by PA2 stream https://youtu.be/qmgZF5ijax8
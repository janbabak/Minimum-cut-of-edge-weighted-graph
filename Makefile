TARGET = pdp

MPIC = mpic++
MPIC_FLAGS = -Wall -pedantic -std=c++20 -fopenmp -O3

MKDIR = mkdir -p
BUILD_DIR = build

.PHONY: all
all: compile

.PHONY: compile
compile: $(TARGET)

.PHONY: run
run: $(TARGET)
	mpirun -np 4 --allow-run-as-root ./$(TARGET) graf_mro/graf_10_5.txt 5 10 974

.PHONY: runMemCheck
runMemCheck: $(TARGET)
	valgrind ./$(TARGET)

$(TARGET): $(BUILD_DIR)/main.o $(BUILD_DIR)/Graph.o $(BUILD_DIR)/TestData.o $(BUILD_DIR)/Edge.o \
$(BUILD_DIR)/ConfigWeight.o $(BUILD_DIR)/ConfigWeightTask.o
	$(MPIC) $(MPIC_FLAGS) $^ -o $@

$(BUILD_DIR)/%.o: src/%.cpp
	$(MKDIR) $(BUILD_DIR)
	$(MPIC) $(MPIC_FLAGS) $< -c -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET) $(BUILD_DIR)/ 2>/dev/null

#depencencies (generated by "g++ -MM src/*.cpp")
$(BUILD_DIR)ConfigWeight.o: src/ConfigWeight.cpp src/ConfigWeight.h src/constants.h
$(BUILD_DIR)ConfigWeightTask.o: src/ConfigWeightTask.cpp src/ConfigWeightTask.h \
 src/ConfigWeight.h src/constants.h
$(BUILD_DIR)Edge.o: src/Edge.cpp
$(BUILD_DIR)Graph.o: src/Graph.cpp src/Graph.h src/Edge.cpp
$(BUILD_DIR)TestData.o: src/TestData.cpp
$(BUILD_DIR)main.o: src/main.cpp src/ConfigWeight.h src/constants.h \
 src/ConfigWeightTask.h src/Graph.cpp src/Graph.h src/Edge.cpp \
 src/TestData.cpp

#inspired by PA2 stream https://youtu.be/qmgZF5ijax8

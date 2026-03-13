
# Makefile for boat_sim project

# Compiler and flags
BIN_PATH = ../mingw64/bin/
CXX = $(BIN_PATH)g++
AS = $(BIN_PATH)as
CXXFLAGS = -Wall -g -mavx -msse4 -DGLM_ENABLE_EXPERIMENTAL -DUSE_SOIL
INC_DIRS = ../include include
INCLUDES = $(addprefix -I,$(INC_DIRS))
LIBS = -lfreeglut -lglew32 -lSOIL -lopengl32 -lglu32
LIB_DIR = ..\libs
LDFLAGS = -L$(LIB_DIR)

# Directories
SRC_DIR = src
BUILD_DIR = build

# Source files - includes .cpp and .s from src and src_project
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/src_project/*.cpp) $(wildcard $(SRC_DIR)/*.s) $(wildcard $(SRC_DIR)/src_project/*.s)

# Object files in build directory - creates corresponding object file paths in build dir
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter $(SRC_DIR)/%.cpp,$(SOURCES)))
OBJECTS += $(patsubst $(SRC_DIR)/%.s,$(BUILD_DIR)/%.o,$(filter $(SRC_DIR)/%.s,$(SOURCES)))


EXECUTABLE = boat_sim
EXECUTABLE_PATH = $(EXECUTABLE)

# Default target
all: $(EXECUTABLE_PATH)

# Rule to create the executable in build dir
$(EXECUTABLE_PATH): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

# General rule to compile/assemble source files to object files in build dir
# For .cpp files in src directory
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@



# For .s files in src directory - USING 'as' directly
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	$(CXX) -g -c $< -o $@ 



# Clean target
clean:
	rm -rf $(BUILD_DIR)

# Debug build target
debug: CXXFLAGS += -DDEBUG -g

# Run target
run: all
	./$(EXECUTABLE_PATH)

# Run debug target
rundebug: debug
	gdb ./$(EXECUTABLE_PATH)

.PHONY: all clean debug run rundebug
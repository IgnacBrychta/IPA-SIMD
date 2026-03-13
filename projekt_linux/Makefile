# Makefile for boat_sim project

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -g -mavx2 -msse4.2 # -Wall for more warnings, -g for debugging symbols
DEPFLAGS = -MMD -MP
INC_DIR = include
SOIL_INCLUDE_HINT_SOIL2 = /usr/include/SOIL2/SOIL2.h
SOIL_INCLUDE_HINT_SOIL = /usr/include/SOIL/SOIL.h

ifeq ($(wildcard $(SOIL_INCLUDE_HINT_SOIL2)), $(SOIL_INCLUDE_HINT_SOIL2))
	CXXFLAGS += -DUSE_SOIL2
	SOIL_LIB = -lSOIL2
else ifeq ($(wildcard $(SOIL_INCLUDE_HINT_SOIL)), $(SOIL_INCLUDE_HINT_SOIL))
	CXXFLAGS += -DUSE_SOIL
	SOIL_LIB = -lSOIL
else
	$(error Could not find SOIL2 or SOIL headers. Install libsoil2-dev or libsoil-dev.)
endif

LIBS = -lglut $(SOIL_LIB) -lGL -lGLEW -lGLU
LIB_DIRS = /usr/lib /usr/lib/x86_64-linux-gnu

LDFLAGS = $(addprefix -L, $(LIB_DIRS))

# Directories
SRC_DIR = src
BUILD_DIR = build

# Source files - includes .cpp and .s from src and src_project
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/src_project/*.cpp) $(wildcard $(SRC_DIR)/*.s) $(wildcard $(SRC_DIR)/src_project/*.s)

# Object files in build directory - creates corresponding object file paths in build dir
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter $(SRC_DIR)/%.cpp,$(SOURCES)))
OBJECTS += $(patsubst $(SRC_DIR)/%.s,$(BUILD_DIR)/%.o,$(filter $(SRC_DIR)/%.s,$(SOURCES)))
DEPS = $(OBJECTS:.o=.d)


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
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -I$(INC_DIR) -c $< -o $@



# For .s files in src directory - USING 'as' directly
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	$(CXX) -g -c $< -o $@ 



# Clean target
clean:
	rm  $(BUILD_DIR)/*
	rm  $(EXECUTABLE_PATH)

# Debug build target
debug: CXXFLAGS += -DDEBUG -g

# Run target
run: all
	./$(EXECUTABLE_PATH)

# Run debug target
rundebug: debug
	gdb ./$(EXECUTABLE_PATH)

.PHONY: all clean debug run rundebug

-include $(DEPS)

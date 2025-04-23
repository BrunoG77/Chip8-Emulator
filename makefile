CXX = g++
INCLUDES = -Iinclude $(shell sdl2-config --cflags)
SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/Chip8/*.cpp)
OBJ = $(SRC:.cpp=.o)
TARGET = chip8

# Compiler flags for each build type
DEBUG_FLAGS = -std=c++17 -Wall -Wextra -Werror $(INCLUDES) -g -DDEBUG
RELEASE_FLAGS = -std=c++17 -Wall -Wextra -Werror $(INCLUDES) -O3

LDFLAGS = $(shell sdl2-config --libs)

.PHONY: all debug release clean

all: debug

debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: $(TARGET)

release: CXXFLAGS = $(RELEASE_FLAGS)
release: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

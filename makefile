CXX = g++
CXXFLAGS = -std=c++2b -Wall -Wextra -Werror
LDFLAGS = `sdl2-config --cflags --libs`

all: chip8

chip8: chip8.cpp
	$(CXX) $(CXXFLAGS) chip8.cpp -o chip8 $(LDFLAGS)

clean:
	rm -f chip8

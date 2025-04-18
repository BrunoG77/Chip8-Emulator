#include "Chip8.hpp"
#include <SDL.h>
#include <fstream>
#include <algorithm>  // For std::copy

// Initialize chip8 machine
void init_chip8(Chip8& chip8, std::string_view rom_name){
    constexpr uint32_t ENTRY_POINT = 0x200; // CHIP8 ROMS will be loaded to 0x200.
                                        // Before that is reserved for the CHIP8 interpreter

    // There are 16 characters at 5 bytes each, so we need an array of 80 bytes.
    constexpr std::array<u_int8_t, 80> FONT_SET =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    // Load font
    std::copy(FONT_SET.begin(), FONT_SET.end(), chip8.ram.begin());

    // Open ROM file, read in binary mode or end it
    std::ifstream rom(rom_name.data(), std::ios::binary | std::ios::ate); 
    if(!rom) {
        throw std::runtime_error("Failed to open ROM: " + std::string(rom_name) + "\n");
    }

    // Get/check rom size
    const size_t rom_size = rom.tellg();
    rom.seekg(0);

    if (rom_size > (chip8.ram.size() - ENTRY_POINT)) {
        throw std::runtime_error("ROM is too big\n");
    }

    // Load ROM
    rom.read(reinterpret_cast<char*>(&chip8.ram[ENTRY_POINT]), rom_size);
    chip8.rom_name = rom_name;
    chip8.PC = ENTRY_POINT;
}

// Handle the input
void handle_input(Chip8& chip8) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                // Exit window. End program
                chip8.state = EmulatorState::QUIT;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    // Exit window if user presses escape
                    chip8.state = EmulatorState::QUIT;
                }
                break;

            default:
                break;
        }
    }
}

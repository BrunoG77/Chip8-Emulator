#include "Chip8.hpp"
#include <SDL.h>
#include <fstream>
#include <algorithm>  // For std::copy
#include <iostream>

namespace Chip8 {
    // Initialize chip8 machine
    void init_chip8(Machine& machine, std::string_view rom_name){
        // Reset in case of reset
        machine.reset();

        constexpr uint32_t ENTRY_POINT = 0x200; // CHIP8 ROMS will be loaded to 0x200.
        // Before that is reserved for the CHIP8 interpreter

        // Font data starts at 0x50 (CHIP-8 specification)
        //CHIP-8 expects fonts at 0x50-0x9F (16 characters × 5 bytes).
        constexpr uint32_t FONTSET_START_ADDRESS = 0x50;

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
        // Copies the font data into the CHIP-8 RAM starting at address 0x50.
        std::copy(FONT_SET.begin(), FONT_SET.end(), machine.ram.begin() + FONTSET_START_ADDRESS);

        // Open ROM file
        // Opens the ROM file as a binary file and puts the file pointer at the end (ate = at end).
        // The | operator combines the two flags, so the file is opened in both binary mode and with the pointer at the end
        std::ifstream rom(rom_name.data(), std::ios::binary | std::ios::ate); 
        if(!rom) {
            throw std::runtime_error("Failed to open ROM: " + std::string(rom_name) + "\n");
        }

        // Get/check rom size
        // Gets the current file pointer position (which is at the end, so this is the file size)
        // static_cast explicitly converts std::streampos(tellg) to size_t.
        const auto rom_size = static_cast<size_t>(rom.tellg());
        rom.seekg(0);   // Moves the file pointer back to the start of the file, ready for reading.

        // Check that the ROM will fit in memory (from ENTRY_POINT to the end of RAM).
        if (rom_size > (machine.ram.size() - ENTRY_POINT)) {
            throw std::runtime_error("ROM exceeds memory bounds\n");
        }

        // Load ROM
        // Reads the ROM file directly into the RAM array, starting at ENTRY_POINT at 0x200
        // data() + ENTRY_POINT is equivalent to &ram[ENTRY_POINT] but more explicit.
        // chip8.ram.data() returns a pointer to the start of the std::array holding the RAM
        // reinterpret_cast<char*> converts the uint8_t* pointer (from your RAM) to a char* pointer,
        // which is what std::ifstream::read expects.
        if (!rom.read(reinterpret_cast<char*>(machine.ram.data() + ENTRY_POINT), rom_size)) {
            throw std::runtime_error("Failed to read entire ROM\n");
        }
        // This is safe here because both uint8_t and char are 1 byte, and its just moving raw data

        machine.rom_name = rom_name;
        machine.PC = ENTRY_POINT; // Program starts at 0x200
    }

    // Handle the input
    // Chip8 original keypad        QWERTY
    // 123C                         1234
    // 456D                         QWER
    // 789E                         ASDF
    // A0BF                         ZXCV
    void handle_input(Machine& machine, Config& config) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    // Exit window. End program
                    machine.state = EmulatorState::QUIT;
                    std::cout << "=== QUIT ===" << std::endl;
                    break;

                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            // Exit window if user presses escape
                            machine.state = EmulatorState::QUIT;
                            std::cout << "=== QUIT ===" << std::endl;
                            break;
                    
                        case SDLK_SPACE:    // Space bar
                            // Toggle state
                            if (machine.state == EmulatorState::RUNNING) {
                                machine.state = EmulatorState::PAUSED;  // Pause
                                std::cout << "=== PAUSED ===" << std::endl;
                            }
                            else
                            {
                                machine.state = EmulatorState::RUNNING; // Resume
                                std::cout << "=== RESUMED ===" << std::endl;
                            }
                            break;
                        
                        case SDLK_l:
                            // "l" will reset CHIP8
                            init_chip8(machine, machine.rom_name);
                            break;
                        
                        case SDLK_o:
                            // "o" will decrease volume
                            if (config.volume > 0) {
                                std::cout << "\nDECREASE VOLUME to: \n" << std::endl;
                                config.volume -= 500;
                                std::cout << config.volume << std::endl;
                            }
                            break;

                        case SDLK_p:
                            // "p" will increase volume
                            if (config.volume < INT16_MAX) {
                                std::cout << "\nINCREASE VOLUME to:\n"<< std::endl;
                                config.volume += 500;
                                std::cout << config.volume << std::endl;
                            }
                            break;
                        
                        // Map qwerty keys to Chip8 COSMAC VIP
                        case SDLK_1: machine.keypad[0x01] = true; break;
                        case SDLK_2: machine.keypad[0x02] = true; break;
                        case SDLK_3: machine.keypad[0x03] = true; break;
                        case SDLK_4: machine.keypad[0x0C] = true; break;

                        case SDLK_q: machine.keypad[0x04] = true; break;
                        case SDLK_w: machine.keypad[0x05] = true; break;
                        case SDLK_e: machine.keypad[0x06] = true; break;
                        case SDLK_r: machine.keypad[0x0D] = true; break;

                        case SDLK_a: machine.keypad[0x07] = true; break;
                        case SDLK_s: machine.keypad[0x08] = true; break;
                        case SDLK_d: machine.keypad[0x09] = true; break;
                        case SDLK_f: machine.keypad[0x0E] = true; break;

                        case SDLK_z: machine.keypad[0x0A] = true; break;
                        case SDLK_x: machine.keypad[0x00] = true; break;
                        case SDLK_c: machine.keypad[0x0B] = true; break;
                        case SDLK_v: machine.keypad[0x0F] = true; break;
                    }
                    break;

                case SDL_KEYUP:
                    // Check if it's the initial press (not held)
                    if (event.key.repeat == 0) {
                        switch(event.key.keysym.sym) {
                            // Map qwerty keys to Chip8 COSMAC VIP
                            case SDLK_1: machine.keypad[0x01] = false; break;
                            case SDLK_2: machine.keypad[0x02] = false; break;
                            case SDLK_3: machine.keypad[0x03] = false; break;
                            case SDLK_4: machine.keypad[0x0C] = false; break;

                            case SDLK_q: machine.keypad[0x04] = false; break;
                            case SDLK_w: machine.keypad[0x05] = false; break;
                            case SDLK_e: machine.keypad[0x06] = false; break;
                            case SDLK_r: machine.keypad[0x0D] = false; break;

                            case SDLK_a: machine.keypad[0x07] = false; break;
                            case SDLK_s: machine.keypad[0x08] = false; break;
                            case SDLK_d: machine.keypad[0x09] = false; break;
                            case SDLK_f: machine.keypad[0x0E] = false; break;

                            case SDLK_z: machine.keypad[0x0A] = false; break;
                            case SDLK_x: machine.keypad[0x00] = false; break;
                            case SDLK_c: machine.keypad[0x0B] = false; break;
                            case SDLK_v: machine.keypad[0x0F] = false; break;
                        }
                    }
                    break;
                break;
            }
        }
    }
}
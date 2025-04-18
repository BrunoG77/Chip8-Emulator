#pragma once
#include "Config.hpp"
#include <array>
#include <string_view>

// Emulator states
enum class EmulatorState {
    // state that emulator is running in
    QUIT,
    RUNNING,
    PAUSED,
};

// Chip8 Machine object
struct Chip8 {
    EmulatorState state = EmulatorState::RUNNING;   // Default machine state

    // Use std::array instead of C-style arrays for type safety and bounds checking.
    std::array<uint8_t, 4096> ram{};  // the ram was 4k. 4096 8bytes

    // 64*32 resolution, cause that is how many pixel we will be emulating. Its easier this way
    // the display was 256 bytes. from 0xF00 to 0xFFF
    std::array<bool, 64 * 32> display{};    // 256 * 8(bytes) = 2048 = 64*32
    // or make display a pointer. DXYN display = &ram[0xF00]; offset after display[10]

    std::array<uint16_t, 12> stack{}; // Subroutine stack. It has 12 levels os stack. 12 bits each I think

    std::array<uint8_t, 16> V{};       // Data registers. V0 - VF.
                        // VF is the carry flag, while in subtraction, it is the "no borrow" flag

    uint16_t I = 0;         // Index register. Which well index memory from

    // Start program counter at ROM entry point. 0x200 is the default entry point. 
    uint16_t PC = 0x200;        // Program counter to know where in memory are we pointing and running opcode

    uint8_t delay_timer = 0;    // Decrements at 60hz when > 0. More for games to move enemy
    uint8_t sound_timer = 0;    // Decrements at 60hz and plays tone when > 0

    std::array<bool, 16> keypad{};    // Hexadecimal keypad 0x0 - 0xF

    // Instead of const char*, use std::string_view for safer string handling.
    std::string_view rom_name;      // To store the name of the rom that is currently loaded
};

// Initialize chip8 machine
void init_chip8(Chip8& chip8, std::string_view rom_name);

// Handle the input
void handle_input(Chip8& chip8);

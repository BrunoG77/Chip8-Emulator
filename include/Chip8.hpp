#pragma once
#include "Config.hpp"
#include <array>
#include <string_view>

namespace Chip8 {
    // Emulator states
    enum class EmulatorState {
        // state that emulator is running in
        QUIT,
        RUNNING,
        PAUSED,
    };

    // Chip8 instruction format
    struct Instruction {
        uint16_t opcode; // 2 byte opcode
        uint16_t NNN;    // 12-bit Address/constant
        uint8_t NN;      // 8-bit constant
        uint8_t N;       // 4-bit constant
        uint8_t X;       // 4-bit register identifier
        uint8_t Y;       // 4-bit register identifier
    };

    // Chip8 Machine object
    struct Machine {
        // Core components
        EmulatorState state = EmulatorState::RUNNING;   // Default machine state

        // Use std::array instead of C-style arrays for type safety and bounds checking.
        std::array<uint8_t, 4096> ram{};  // the ram was 4k. 4096 8bytes

        // 64*32 resolution, cause that is how many pixel we will be emulating. Its easier this way
        // the display was 256 bytes. from 0xF00 to 0xFFF
        std::array<bool, 64 * 32> display{};    // 256 * 8(bytes) = 2048 = 64*32
        // or make display a pointer. DXYN display = &ram[0xF00]; offset after display[10]


        // Registers
        std::array<uint8_t, 16> V{};       // Data registers. V0 - VF.
                            // VF is the carry flag, while in subtraction, it is the "no borrow" flag

        uint16_t I = 0;         // Index register. Which well index memory from

        // Start program counter at ROM entry point. 0x200 is the default entry point. 
        uint16_t PC = 0x200;        // Program counter to know where in memory are we pointing and running opcode


        // Stack
        std::array<uint16_t, 16> stack{}; // Subroutine stack. It has up to 12 levels of nesting. 12 or 16 bits each
        uint8_t stack_ptr = 0;    // Make stack_ptr an integer index for stack operations like push/pop.
        // The stack pointer (SP) just keeps track of where the top of the stack is 
        // Since the stack is at most 16 elements, an 8-bit integer (0–255) is more than enough to index into it.


        // Timers
        uint8_t delay_timer = 0;    // Decrements at 60hz when > 0. More for games to move enemy
        uint8_t sound_timer = 0;    // Decrements at 60hz and plays tone when > 0


        // Input
        std::array<bool, 16> keypad{};    // Hexadecimal keypad 0x0 - 0xF


        // System
        // Instead of const char*, use std::string_view for safer string handling.
        std::string_view rom_name;      // To store the name of the rom that is currently loaded

        Instruction current_inst{};     // Currently executing instruction
    };

    // Initialization and core functions
    void init_chip8(Machine& machine, std::string_view rom_name);

    // Handle the input
    void handle_input(Machine& machine);
}

#include "Chip8/Cpu.hpp"
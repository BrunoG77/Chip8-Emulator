#include "Chip8/Cpu.hpp"
#include <stdexcept>

#ifdef DEBUG
    #include <iostream>
#endif

namespace Chip8 {
    #ifdef DEBUG
        std::string opcode_description(uint16_t opcode) {
            switch (opcode & 0xF000) {
                case 0x0000:
                    if ((opcode & 0x00FF) == 0xE0) return "CLS (Clear the display)";
                    if ((opcode & 0x00FF) == 0xEE) return "RET (Return from subroutine)";
                    return "SYS (Ignored)";
                case 0x1000: return "JP addr (Jump to address)";
                case 0x2000: return "CALL addr (Call subroutine)";
                case 0x3000: return "Skips next instruction if VX == NN";
                case 0x4000: return "Skips next instruction if VX != NN";
                case 0x5000: return "Skips next instruction if VX == VY";
                case 0x6000: return "Sets VX to NN";
                case 0x7000: return "Adds NN to VX (carry flag is not changed)";

                case 0x9000: return "Skips the next instruction if VX does not equal VY";
                case 0xA000: return "Sets I to the address NNN";
                case 0xB000: return "Jumps to the address NNN plus V0";
                case 0xC000: return "Sets VX to the result of a bitwise and operation "
                                        "on a random number (Typically: 0 to 255) and NN";
                case 0xD000: return "Draws a sprite at coordinate (VX, VY) "
                                        "that has a width of 8 pixels and a height of N pixels";
                default: return "Unknown/Unimplemented opcode";
            }
        }
    #endif

    // Emulate 1 machine instruction
    void emulate_instruction(Machine& machine, const Config config) {
        // x86 is small indian architecture
        // PC is in its instruction address gathering data in 2 bytes big indian
        // We need grab the first byte, so shift it over to the left, grab it and or that in
        // For it to read and execute as a big indian value
        // Get next opcode from RAM
        machine.current_inst.opcode = 
            (machine.ram[machine.PC] << 8) | machine.ram[machine.PC+1];

        #ifdef DEBUG
            std::cout << "PC: 0x" << std::hex << machine.PC
                  << "  OPCODE: 0x" << machine.current_inst.opcode
                  << "  DESC: " << opcode_description(machine.current_inst.opcode)
                  << std::dec << std::endl;
        #endif

        // To read the next opcode on the next go around, increase PC by 2 bytes
        machine.PC +=2;  // Pre-increment PC for next opcode, instead of incrementing it later

        // Fill out current instruction format
        // DXYN is the display opcode || X and Y always appear on those positions like the skip opcode EX9E
        // To get Y or X we need to shit to the right and then mask those bits
        machine.current_inst.NNN = machine.current_inst.opcode & 0x0FFF;
        machine.current_inst.NN = machine.current_inst.opcode & 0x00FF;
        machine.current_inst.N = machine.current_inst.opcode & 0x000F;
        machine.current_inst.Y = (machine.current_inst.opcode >> 4) & 0x000F;
        machine.current_inst.X = (machine.current_inst.opcode >> 8) & 0x000F; 


        // Emulate the 35 opcodes
        switch (machine.current_inst.opcode >> 12)
        {
        case 0x00:
            // if else because there are only 2 cases where they start with 0
            if (machine.current_inst.NN == 0xE0) {
                // 0x00E0: Clear the screen
                machine.display.fill(false);
            } else if (machine.current_inst.NN == 0xEE)
            {
                // 0x00EE: Returns from a subroutine.
                // Set PC to last address on subroutine stack ("pop" it off the stack)
                //  so that next opcode will be gotten from that address.
                // cause it was incremented, we need the decremented value
                machine.PC = machine.stack[--machine.stack_ptr]; 
                // The stack_ptr is decremented first (--) to point to the most recently pushed value.
                // The value at that index is retrieved from the stack and assigned to the program counter
            }
            break;

        case 0x01:
            // 1NNN: Jumps to address NNN;
            machine.PC = machine.current_inst.NNN;  // Set Program counter so that next opcode is from NNN

            // DEBUG
            #ifdef DEBUG
                std::cout << "Jump to NNN: " << std::hex << machine.current_inst.NNN 
                << std::dec << std::endl;
            #endif
            break;

        case 0x02:
            if (machine.stack_ptr >= machine.stack.size()) {
                throw std::runtime_error("Stack overflow");
            }

            // 2NNN: Calls subroutine at NNN
            // the current executing address for this opcode, we already incremented past it
            // we're pointing to the next one, thats where we'll need to return from the subroutine to keep executing
            // if we didn't pre-increment, we'd be adding the call to the subroutine stack and enter a infinite loop
            machine.stack[machine.stack_ptr++] = machine.PC; 
            // ("push" it on the stack) PC is stored at the stack_ptr index and then increment it (++)

            // Now make PC NNN cause that is where the subroutine is, so that next opcode is gotten from there
            machine.PC = machine.current_inst.NNN;
            break;


        case 0x06:
            // 0x6XNN: Set register VX to NN
            machine.V[machine.current_inst.X] = machine.current_inst.NN;
            break;

        case 0x07:
            // 0x7XNN: Adds NN to VX (carry flag is not changed)
            machine.V[machine.current_inst.X] += machine.current_inst.NN;
            break;

        case 0x0A:
            // 0xANNN: Set index register I to NNN
            machine.I = machine.current_inst.NNN;
            break;

        
            case 0x0D: {
                // 0xDXYN: Draw N- height sprite at coordinates X,Y 
                // Read from memory location I
                // Screen pixels are XOR'd with sprite bits, 
                // VF (carry flag) is set if any screen pixels are set off
                // This is useful for collision detection or other reasons
                // V[X] modulo(%) 64(resolution window width)
                uint8_t X_coord = machine.V[machine.current_inst.X] % config.window_width;
                uint8_t Y_coord = machine.V[machine.current_inst.Y] % config.window_height;
                const uint8_t orig_X = X_coord; // Original X coordinate
                
                machine.V[0xF] = 0;    // Initialize carry flag to 0
            
                // Read each row of the sprite and loop over all N rows of the sprite
                for (uint8_t i = 0; i < machine.current_inst.N; i++) {
                    // Get next byte/row of sprite data
                    const uint8_t sprite_data = machine.ram[machine.I + i]; // i is the offset
            
                    X_coord = orig_X;   // Reset X for next row to draw
            
                    // Check all 8 bits of row
                    for (int8_t j = 7; j >= 0; j--) {
                        bool pixel = machine.display[Y_coord * config.window_width + X_coord];
                        const bool sprite_bit = (sprite_data & (1 << j));
                        // Check if bit is on. Testing the bit left to right
                        // 1 << 7 (1 shift left by seven) will be the top most bit
                        // If sprite pixel/bit is on and display pixel is on, set carry flag
                        if (sprite_bit && pixel) {
                            machine.V[0xF] = 1; // Set
                        }
            
                        // XOR display pixel with sprite pixel/bit to set it on/off
                        pixel ^= sprite_bit;
            
                        // Update the display pixel
                        machine.display[Y_coord * config.window_width + X_coord] = pixel;
            
                        // Stop drawing if hits the right edge of the screen
                        if (++X_coord >= config.window_width) break;
                    }
            
                    // Stop drawing the entire sprite if it hits the bottom edge of the screen
                    if (++Y_coord >= config.window_height) break;
                }

                // DEBUG
                #ifdef DEBUG
                    std::cout << "DRAW: X=" << static_cast<uint16_t>(machine.current_inst.X)
                            << " Y=" << static_cast<uint16_t>(machine.current_inst.Y)
                            << " N=" << static_cast<uint16_t>(machine.current_inst.N)
                            << " V[X]=" << static_cast<uint16_t>(machine.V[machine.current_inst.X])
                            << " V[Y]=" << static_cast<uint16_t>(machine.V[machine.current_inst.Y])
                            << " I=0x" << std::hex << machine.I << std::dec
                            << " Sprite Data:";
                    for (int i = 0; i < machine.current_inst.N; ++i) {
                        std::cout << " " << std::hex << static_cast<uint16_t>(machine.ram[machine.I + i]);
                    }
                    std::cout << std::endl;
                #endif

                break;
            }
            
        
        default:
            throw std::runtime_error("Unimplemented opcode");
        }
    }
}
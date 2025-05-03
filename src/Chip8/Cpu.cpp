#include "Chip8/Cpu.hpp"
#include <stdexcept>
#include <iostream>

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
                case 0x8000:
                    if ((opcode & 0x000F) == 0x0) return "Sets VX to the value of VY";
                    if ((opcode & 0x000F) == 0x1) return "Sets VX to VX or VY";
                    if ((opcode & 0x000F) == 0x2) return "Sets VX to VX and VY";
                    if ((opcode & 0x000F) == 0x3) return "Sets VX to VX xor VY";
                    if ((opcode & 0x000F) == 0x4) return "Adds VY to VX";
                    if ((opcode & 0x000F) == 0x5) return "VY is subtracted from VX";
                    if ((opcode & 0x000F) == 0x6) return "Shifts VX to the right by 1";
                    if ((opcode & 0x000F) == 0x7) return "Sets VX to VY minus VX";
                    if ((opcode & 0x000F) == 0xE) return "Shifts VX to the left by 1";
                    return "Wrong/Unimplemented Opcode";
                case 0x9000: return "Skips the next instruction if VX does not equal VY";
                case 0xA000: return "Sets I to the address NNN";
                case 0xB000: return "Jumps to the address NNN plus V0";
                case 0xC000: return "Sets VX to the result of a bitwise and operation "
                                        "on a random number (Typically: 0 to 255) and NN";
                case 0xD000: return "Draws a sprite at coordinate (VX, VY) "
                                        "that has a width of 8 pixels and a height of N pixels";
                case 0xE000:
                    if ((opcode & 0x00FF) == 0x9E) return "Skip the next instruction if the key stored in VX"
                                                            " is pressed";
                    if ((opcode & 0x00FF) == 0xA1) return "Skip the next instruction if the key stored in VX" 
                                                            " is not pressed";
                    return "Wrong/Unimplemented Opcode";
                case 0xF000:
                    if ((opcode & 0x00FF) == 0x07) return "Sets VX to the value of the delay timer";
                    if ((opcode & 0x00FF) == 0x0A) return "A key press is awaited, and then stored in VX";
                    if ((opcode & 0x00FF) == 0x15) return "Sets the delay timer to VX";
                    if ((opcode & 0x00FF) == 0x18) return "Sets the sound timer to VX";
                    if ((opcode & 0x00FF) == 0x1E) return "Adds VX to I. For non-Amiga Chip8, VF is not affected";
                    if ((opcode & 0x00FF) == 0x29) return "Sets I to the location of the sprite in memory"
                                                            " for the character in VX";
                    if ((opcode & 0x00FF) == 0x33) return "Stores the binary-coded decimal representation of VX"
                                                            " at memory offset from I";
                    if ((opcode & 0x00FF) == 0x55) return "Stores from V0 to VX (including VX) in memory,"
                                                            " starting at address I";
                    if ((opcode & 0x00FF) == 0x65) return "Fills from V0 to VX (including VX) with values from memory,"
                                                            " starting at address I ";
                    return "Wrong/Unimplemented Opcode";
            }
            return "Unknown/Unimplemented opcode";
        }
    #endif

    // Emulate 1 machine instruction
    void emulate_instruction(Machine& machine, const Config& config) {
        bool carry = 0;
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
            } else
            {
                std::cout << "CALL not implemented because its not necessary for most ROMs"
                << std::endl;
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

        case 0x03:
            // 0x3XNN: Skips the next instruction if VX equals NN 
            // (usually the next instruction is a jump to skip a code block)
            if (machine.V[machine.current_inst.X] == machine.current_inst.NN) {
                machine.PC += 2;    // Skip to next opcode (2bytes)
            }
            break;
        
        case 0x04:
            // 0x4XNN: Skips the next instruction if VX does not equal NN 
            // (usually the next instruction is a jump to skip a code block)
            if (machine.V[machine.current_inst.X] != machine.current_inst.NN) {
                machine.PC += 2;    // Skip to next opcode (2bytes)
            }
            break;

        case 0x05:
            // 0x5XY0: Skips the next instruction if VX equals VY
            // (usually the next instruction is a jump to skip a code block)
            if (machine.current_inst.N != 0) {
                // If its not 0, then its the wrong opcode
                std::cout << "0x5XY0 -> N is not 0. Wrong Opcode" << std::endl;
                break;
            }

            if (machine.V[machine.current_inst.X] == machine.V[machine.current_inst.Y]) {
                machine.PC += 2;    // Skip to next opcode (2bytes)
            }
            break;

        case 0x06:
            // 0x6XNN: Set register VX to NN
            machine.V[machine.current_inst.X] = machine.current_inst.NN;
            break;

        case 0x07:
            // 0x7XNN: Adds NN to VX (carry flag is not changed)
            machine.V[machine.current_inst.X] += machine.current_inst.NN;
            break;

        case 0x08:
            switch (machine.current_inst.N) {
                case 0x0:
                    // 0x8XY0: Sets VX to the value of VY
                    machine.V[machine.current_inst.X] = machine.V[machine.current_inst.Y];
                    break;
                
                case 0x1:
                    // 0x8XY1: Sets VX to VX or VY. (bitwise OR operation)
                    machine.V[machine.current_inst.X] |= machine.V[machine.current_inst.Y];
                    // In original behaviour, in 8XY1/2/3, it reset the carry flag
                    machine.V[0xF] = 0;
                    break;
                
                case 0x2:
                    // 0x8XY2: Sets VX to VX and VY. (bitwise AND operation)
                    machine.V[machine.current_inst.X] &= machine.V[machine.current_inst.Y];
                    machine.V[0xF] = 0;
                    break;
                
                case 0x3:
                    // 0x8XY3: Sets VX to VX xor VY
                    machine.V[machine.current_inst.X] ^= machine.V[machine.current_inst.Y];
                    machine.V[0xF] = 0;
                    break;

                case 0x4:
                    // 0x8XY4: Adds VY to VX. 
                    // VF is set to 1 when there's an overflow, and to 0 when there is not
                    #ifdef DEBUG
                    std::cout << "V[X]: " << static_cast<uint16_t>(machine.V[machine.current_inst.X])
                    << " V[Y]: " << static_cast<uint16_t>(machine.V[machine.current_inst.Y]) << " VF: " 
                    << static_cast<uint16_t>(machine.V[0xF]) << std::endl;
                    #endif

                    carry = ((machine.V[machine.current_inst.X] 
                                + machine.V[machine.current_inst.Y]) 
                                > 0xFF);

                    machine.V[machine.current_inst.X] += machine.V[machine.current_inst.Y];

                    machine.V[0xF] = carry;

                    #ifdef DEBUG
                    std::cout << "After Sum -> " << "V[X]: " 
                    << static_cast<uint16_t>(machine.V[machine.current_inst.X])
                    << " V[Y]: " << static_cast<uint16_t>(machine.V[machine.current_inst.Y]) << " VF: " 
                    << static_cast<uint16_t>(machine.V[0xF]) << std::endl;
                    #endif

                    break;

                case 0x5:
                    // 0x8XY5: VY is subtracted from VX. 
                    // VF is set to 0 when there's an underflow, 1 when there is not 
                    #ifdef DEBUG
                    std::cout << "V[X]: " << static_cast<int16_t>(machine.V[machine.current_inst.X])
                    << " V[Y]: " << static_cast<int16_t>(machine.V[machine.current_inst.Y]) << " VF: " 
                    << static_cast<int16_t>(machine.V[0xF]) << std::endl;
                    #endif

                    // Get carry value first, then do operation and ONLY after set the carry flag
                    // It's the correct order for the CHIP8 interpreter
                    carry = (machine.V[machine.current_inst.Y] <= machine.V[machine.current_inst.X]);

                    machine.V[machine.current_inst.X] -= machine.V[machine.current_inst.Y];

                    machine.V[0xF] = carry;
                        // V[Y] is bigger then V[X] So the result will be negative and underflow (borrow)
                    
                    #ifdef DEBUG
                    std::cout << "After Subtraction VX=VX-VY -> " << "V[X]: " 
                    << static_cast<int16_t>(machine.V[machine.current_inst.X])
                    << " V[Y]: " << static_cast<int16_t>(machine.V[machine.current_inst.Y]) << " VF: " 
                    << static_cast<int16_t>(machine.V[0xF]) << std::endl;
                    #endif
                    break;

                case 0x6:
                    // 0x8XY6: Shifts VX to the right by 1, 
                    // then stores the least significant bit of VX prior to the shift into VF.
                    // X is a 4-bit register identifier so if its 10 -> 1010, we store 0
                    carry = machine.V[machine.current_inst.Y] & 1;  // Use V[Y] instead of X for Chip8

                    // shift right so the 10 -> 1010 will now be 5 -> 0101
                    machine.V[machine.current_inst.X] = machine.V[machine.current_inst.Y] >> 1;

                    machine.V[0xF] = carry;

                    // SOME VARIANTS
                    /*
                    machine.V[0xF] = machine.V[machine.current_inst.Y] & 1; 

                    machine.V[machine.current_inst.X] = machine.V[machine.current_inst.Y] >> 1;
                    */
                    break;

                case 0x7:
                    // 0x8XY7: Sets VX to VY minus VX. 
                    // VF is set to 0 when there's an underflow, and 1 when there is not 
                    #ifdef DEBUG
                    std::cout << "V[X]: " << static_cast<int16_t>(machine.V[machine.current_inst.X])
                    << " V[Y]: " << static_cast<int16_t>(machine.V[machine.current_inst.Y]) << " VF: " 
                    << static_cast<int16_t>(machine.V[0xF]) << std::endl;
                    #endif

                    machine.V[machine.current_inst.X] = machine.V[machine.current_inst.Y] - 
                                                        machine.V[machine.current_inst.X];

                    if (machine.V[machine.current_inst.Y] >= machine.V[machine.current_inst.X]){
                        machine.V[0xF] = 1;
                    } else {
                        // V[Y] is bigger then V[X] So the result will be negative and underflow (borrow)
                        machine.V[0xF] = 0; // Underflow
                    }

                    #ifdef DEBUG
                    std::cout << "After Subtraction VX=VY-VX -> " << "V[X]: " 
                    << static_cast<int16_t>(machine.V[machine.current_inst.X])
                    << " V[Y]: " << static_cast<int16_t>(machine.V[machine.current_inst.Y]) << " VF: " 
                    << static_cast<int16_t>(machine.V[0xF]) << std::endl;
                    #endif
                    break;

                case 0xE:
                    // 0x8XYE: Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX 
                    // prior to that shift was set, or to 0 if it was unset
                    // Use V[Y]
                    carry = (machine.V[machine.current_inst.Y] & 0x80) >> 7; // isolate most significant bit

                    machine.V[machine.current_inst.X] = machine.V[machine.current_inst.Y] << 1;

                    machine.V[0xF] = carry;
                    break;

                default:
                    // Wrong/unimplemented opcode
                    break;
            }
            break;
        
        case 0x09:
            // 0x9XY0: Skips the next instruction if VX does not equal VY
            // (usually the next instruction is a jump to skip a code block)
            if (machine.current_inst.N != 0) {
                // If its not 0, then its the wrong opcode
                #ifdef DEBUG
                std::cout << "0x9XY0 -> N is not 0. Wrong Opcode" << std::endl;
                #endif
                break;
            }

            if (machine.V[machine.current_inst.X] != machine.V[machine.current_inst.Y]) {
                machine.PC += 2;    // Skip to next opcode (2bytes)
            }
            break;

        case 0x0A:
            // 0xANNN: Set index register I to NNN
            machine.I = machine.current_inst.NNN;
            break;

        case 0x0B:
            // BNNN: Jumps to the address NNN plus V0;
            machine.PC = machine.current_inst.NNN + machine.V[0x0];

            // DEBUG
            #ifdef DEBUG
                std::cout << "NNN: " << std::hex << machine.current_inst.NNN 
                << " V0: " << machine.V[0x0]
                << " Jump to NNN + V0: " << machine.PC
                << std::dec << std::endl;
            #endif
            break;
    
        case 0x0C:
            // CXNN: Sets VX to the result of a bitwise and operation on a random number and NN
            // the Random number typically is 0 to 255, so do rand with a modulo of 256
            machine.V[machine.current_inst.X] = (rand() % 256) & machine.current_inst.NN;
            break;

        case 0x0D: {
            // 0xDXYN: Draw N- height sprite at coordinates X,Y 
            // Read from memory location I
            // Screen pixels are XOR'd with sprite bits, 
            // VF (carry flag) is set if any screen pixels are set off
            // This is useful for collision detection or other reasons
            // V[X] modulo(%) 64(resolution window width) Modulo ensures coordinates wrap around the screen
            // If X or Y is larger than the display width/height, it wraps back to zero.
            // If X = 66 and window_width = 64, 66 % 64 = 2 → pixel is drawn at column 2, not 66.
            uint8_t X_coord = machine.V[machine.current_inst.X] % config.window_width;  // gives the X position 
            uint8_t Y_coord = machine.V[machine.current_inst.Y] % config.window_height; // gives the Y position 
            const uint8_t orig_X = X_coord; // Original X coordinate
            
            machine.V[0xF] = 0;    // Initialize carry flag to 0
        
            // Read each row of the sprite and loop over all N rows of the sprite (height N)
            // Each row is a byte in memory starting at address I
            for (uint8_t i = 0; i < machine.current_inst.N; i++) {
                // Get next byte/row of sprite data
                const uint8_t sprite_data = machine.ram[machine.I + i]; // i is the offset
        
                X_coord = orig_X;   // Reset X for next row to draw
        
                // Loop over each bit in the sprite byte (width 8), Check all 8 bits of row
                // Bit index within the sprite byte (from 7 down to 0, left to right).
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

        case 0x0E:
            if (machine.current_inst.NN == 0x9E) {
                // 0xEX9E: Skips the next instruction if the key stored in VX(only check lowest nibble) is pressed
                // (usually the next instruction is a jump to skip a code block)
                if (machine.keypad[machine.V[machine.current_inst.X]]) {
                    machine.PC += 2;
                }
            } else if (machine.current_inst.NN == 0xA1)
            {
                // 0xEXA1: Skips the next instruction if the key stored in VX(lowest nibble) is not pressed
                if (!machine.keypad[machine.V[machine.current_inst.X]]) {
                    machine.PC += 2;
                }
            } else
            {
                #ifdef DEBUG
                std::cout << "Opcode not implemented/wrong"
                << std::endl;
                #endif
            }
            break;

        case 0x0F:
            switch (machine.current_inst.NN)
            {
            case 0x07:
                // 0xFX07: Sets VX to the value of the delay timer
                machine.V[machine.current_inst.X] = machine.delay_timer;
                break;

            case 0x0A: {
                // 0xFX0A: A key press is awaited, and then stored in VX 
                // (blocking operation, all instruction halted until next key event, 
                // delay and sound timers should continue processing)
                bool any_key_pressed = false;
                uint8_t key_pressed = 0xFF;

                for (uint8_t i = 0; key_pressed == 0xFF && i < machine.keypad.size(); i++) {
                    if (machine.keypad[i]) {
                        key_pressed = i;    // save pressed key to check until its released

                        any_key_pressed = true;
                        break;
                    }

                    if (!any_key_pressed) {
                        machine.PC -= 2;    // Keep getting the current opcode to wait for key press
                        break;
                    } else {
                        // Key has been pressed, but wait until its released to store value
                        if (machine.keypad[key_pressed]) {
                            machine.PC -= 2;
                        } else {
                            machine.V[machine.current_inst.X] = key_pressed;    // VX = key pressed

                            // Reset key
                            key_pressed = 0xFF;
                            any_key_pressed = false;
                        }
                    }
                }
                break;
            }

            case 0x15:
                // 0xFX15: Sets the delay timer to VX
                machine.delay_timer = machine.V[machine.current_inst.X];
                break;

            case 0x18:
                // 0xFX18: Sets the sound timer to VX
                machine.sound_timer = machine.V[machine.current_inst.X];
                break;

            case 0x1E:
                // 0xFX1E: Adds VX to I. For non-Amiga Chip8, VF is not affected
                machine.I += machine.V[machine.current_inst.X];
                break;

            case 0x29:
                // 0xFX29: Sets I to the location of the sprite in memory for the character in VX(0x0-0xF)
                // Characters 0-F (in hexadecimal) are represented by a 4x5 font (4-bits 5-bytes)
                // ADD 0x50 because my font starts at that address
                machine.I = 0x50 + (machine.V[machine.current_inst.X] * 5);
                break;

            case 0x33:{
                // 0xFX33: Stores the binary-coded decimal representation of VX at memory offset from I
                // I = hundreds place, I+1 = tens place, I+2 = ones place 
                // Binary code: tetris score 0010 0111 1000 -> Score: 278
                uint8_t bcd = machine.V[machine.current_inst.X]; // e.g 123
                machine.ram[machine.I+2]= bcd % 10; // 12[3]

                bcd /= 10;  // divide by 10 to get rid of last digit
                machine.ram[machine.I+1]= bcd % 10; // 1[2]

                bcd /= 10;
                machine.ram[machine.I]= bcd % 10; // [1]
                
                break;
            }

            case 0x55:
                // 0xFX55: Stores from V0 to VX (including VX) in memory, starting at address I 
                // The offset from I is increased by 1 for each value written, but I itself is left unmodified
                // SCHIP does not increment I, Chip8 does increment I
                for (uint8_t i = 0; i <= machine.current_inst.X; i++) {
                    machine.ram[machine.I++] = machine.V[i]; // Increment I for Chip8
                }
                break;

            case 0x65:
                // 0xFX65: Fills from V0 to VX (including VX) with values from memory, starting at address I 
                // The offset from I is increased by 1 for each value read, but I itself is left unmodified
                for (uint8_t i = 0; i <= machine.current_inst.X; i++) {
                    machine.V[i] = machine.ram[machine.I++];
                }
                break;
            }
            break;
            
        default:
            throw std::runtime_error("Unimplemented opcode");
        }
    }
}
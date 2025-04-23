#include "SDLManager.hpp"
#include "Chip8.hpp"
// std::cout and such
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        // Check for ROM argument FIRST before any initialization
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <rom_path>" << std::endl;
            return EXIT_FAILURE;  // Exit immediately
        }

        // Get initial config
        Config config;

        // Initialize SDL with RAII
        Chip8::SDLManager sdl(config);
        std::cout << "SDL Initialized" << std::endl;

        // Initialize chip8
        Chip8::Machine machine;
        init_chip8(machine, argv[1]);
        
        // Initial screen clear
        sdl.clear_window();
        
        // Main emulator Loop
        // Chip8 has an instruction to conditionally clear the screen
        while (machine.state != Chip8::EmulatorState::QUIT) {
            // Handle input
            handle_input(machine);

            if (machine.state == Chip8::EmulatorState::PAUSED){
                continue;
            }

            // Get_time();
            // Emulate CHIP8 instructions
            emulate_instruction(machine, config);

            // Get_time() elapsed since last Get_time();
            //
            // Delay for approximately 60HZ/60FPS, because the CRT TV was 60HZ back then
            // 16 because there was no float number, for 60HZ it is 16.67ms
            SDL_Delay(16);

            // Update window by presenting it
            sdl.update_window(config, machine);
        }
        
        std::cout << "Emulator shut down successfully" << std::endl;
        return EXIT_SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

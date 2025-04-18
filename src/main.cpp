#include "SDLManager.hpp"
#include "Chip8.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            (void) argc;
            (void) argv;
        }
        // Get initial config
        Config config;

        // Initialize SDL with RAII
        SDLManager sdl(config);
        std::cout << "SDL Initialized" << std::endl;

        // Initialize chip8
        Chip8 chip8;
        init_chip8(chip8, argv[1]);
        
        // Initial screen clear
        sdl.clear_window();
        
        // Main emulator Loop
        // Chip8 has an instruction to conditionally clear the screen
        while (chip8.state != EmulatorState::QUIT) {
            // Handle input
            handle_input(chip8);

            // Get_time();
            // Emulate CHIP8 instructions
            // Get_time() elapsed since last Get_time();
            //
            // Delay for approximately 60HZ/60FPS, because the CRT TV was 60HZ back then
            // 16 because there was no float number, for 60HZ it is 16.67ms
            SDL_Delay(16);

            // Update window by presenting it
            sdl.update_window();
        }
        
        std::cout << "Emulator shut down successfully\n";
        return EXIT_SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

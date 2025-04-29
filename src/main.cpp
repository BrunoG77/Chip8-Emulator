#include "SDLManager.hpp"
#include "Chip8.hpp"
// std::cout and such
#include <iostream>
#include <time.h>
#include <chrono> // For precise timing

using namespace std::chrono;

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

        // Seed random number generator
        srand(time(NULL));

        // Timing variables (local to main)
        // CHIP-8 Timers run at 60 Hz, independent of the CPU speed/frame rate
        // These variables store the last time we updated the timers and ran a CPU instruction
        // auto last_timer_tick = steady_clock::now();
        auto last_loop_time = steady_clock::now();

        // How many instructions per second you want to execute (configurable)
        const int cpu_hz = config.ints_per_second;

        // CHIP-8 timers always tick at 60Hz, because the CRT TV was 60HZ back then
        const int timer_hz = 60;

        // How much time (in seconds) should pass between each CPU instruction or timer tick
        // for 60HZ it is 16.67ms
        double cpu_period = 1.0 / cpu_hz;       // Seconds per CPU instruction
        double timer_period = 1.0 / timer_hz;   // Seconds per timer tick (should be ~0.01667)

        // These track the fractional time that has passed, 
        // so it runs the right number of instructions/timer ticks 
        // even if the loop timing is not perfectly regular.
        // Accumulators
        double cpu_accum = 0.0;
        double timer_accum = 0.0;
        
        // Main emulator Loop
        // Chip8 has an instruction to conditionally clear the screen
        while (machine.state != Chip8::EmulatorState::QUIT) {
            // Time for input
            handle_input(machine);

            if (machine.state == Chip8::EmulatorState::PAUSED){
                // Sleep a bit to avoid 100% CPU usage
                SDL_Delay(10);
                continue;
            }

            // Calculate elapsed time
            // Get the current time point (high-resolution, monotonic clock)
            auto now = steady_clock::now();

            // Calculate how much time (in seconds
            // (now - last_cpu_tick gives a duration; duration<double> converts it to seconds as a double)
            // ) has passed since the last loop iteration
            double elapsed = duration<double>(now - last_loop_time).count();

            // Update the reference point for the next loop
            last_loop_time = now;

            // Know how much time has passed since last CPU instruction and timer update
            cpu_accum += elapsed;
            timer_accum += elapsed;

            // Run CPU instructions at the configured rate
            while (cpu_accum >= cpu_period) {
                emulate_instruction(machine, config);
                cpu_accum -= cpu_period;
            }

            // Update timers at 60Hz
            // timer_period is how long (in seconds) should pass between each timer tick 
            // (for 60Hz, timer_period = 1.0 / 60)
            // If enough time has passed for a timer tick, decrement the delay and sound timers
            if (timer_accum >= timer_period) {
                // Opcode 0xFX15 sets the delay timer as V[X]
                // Decrement if its above 0
                if (machine.delay_timer > 0) --machine.delay_timer;

                // Opcode 0xFX18 sets the sound timer as V[X]
                if (machine.sound_timer > 0) --machine.sound_timer;

                // call to play or pause
                sdl.handle_audio(machine);

                // Subtract the period from the accumulator so it can handle multiple ticks if the loop is slow
                timer_accum -= timer_period;
            }

            // e.g
            // If the loop is slow:
            // If it takes 40ms (0.04s) instead of the ideal 16.67ms (0.01667s)
            // cpu_accum will be 0.04s, so if the CPU period is 0.001s (1000Hz), itl run 40 instructions to catch up
            // timer_accum will be 0.04s, so itl decrement the timers 2 times (since 0.04 / 0.01667 ≈ 2.4)


            // Render the screen (can be tied to timer or every frame)
            sdl.update_window(config, machine);

            // Sleep a little to avoid 100% CPU usage
            SDL_Delay(1);
            
        }
        
        std::cout << "Emulator shut down successfully" << std::endl;
        return EXIT_SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

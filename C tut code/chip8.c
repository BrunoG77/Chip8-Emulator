#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL.h>

// SDL Container object
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_t;

// Emulator configuration object
typedef struct {
    uint32_t window_width;      // SDL window width
    uint32_t window_height;     // SDL window height
    uint32_t fg_color;          // Foreground color RGBA8888
    uint32_t bg_color;          // Background color RGBA8888
    uint32_t scale_factor       // Amount to scale a CHIP8 pixel by e.g. 20x will be a 20x larger window
} config_t; 

// Emulator states
typedef enum {
    // state that emulator is running in
    QUIT,
    RUNNING,
    PAUSED,
} emulator_state_t;

// Chip8 Machine object
typedef struct
{
    emulator_state_t state;
} chip8_t;


// Initialize SDL
bool init_sdl(sdl_t *sdl, config_t *config) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Could not initialize SDL subsystems! %s\n", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow("CHIP8 Emulator", SDL_WINDOWPOS_CENTERED, 
                                   SDL_WINDOWPOS_CENTERED, 
                                   config.window_width * config.scale_factor, 
                                   config.window_height * config.scale_factor, 
                                   0);
    if (!sdl->window) {
        SDL_Log("Could not create SDL window %s\n", SDL_GetError());
        return false;
    }

    sdl->renderer = SDL_CreateRenderer(sdl->window, -1, SDL_RENDERER_ACCELERATED);
    if (!sdl->renderer) {
        SDL_Log("Could not create SDL renderer %s\n", SDL_GetError());
        return false;
    }

    return true;    // Success
}

// Set up initial emulator configuration from passed in arguments
bool set_config_from_args(config_t *config, const int argc, char **argv) {

    // Set defaults
    *config = (config_t){
        .window_width  = 64,    // CHIP8 original X resolution
        .window_height = 32,    // CHIP8 original Y resolution
        .fg_color = 0xFFFFFFFF, // White
        .bg_color = 0xFFFF00FF, // Yellow
        .scale_factor = 20,     // Default resolution will now be 1280*640
    };

    // Override defaults from passed in arguments
    for (int i = 1; i < argc; i++) {
            (void)argv[i];  // Prevent compiler error from unused variables argc/argv
    }

    return true;    // Success
}

// Initialize chip8 machine
bool init_chip8(chip8_t *chip8){
    chip8->state = RUNNING; // Default machine state
    return true;    // Success
}

// Final cleanup
void final_cleanup(const sdl_t sdl) {
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
    SDL_Quit(); // Shut down SDL subsystem
}

// Clear screen / SDL Window to background color
void clear_screen(const sdl_t sdl, const config_t config) {
    const uint8_t r = (config.bg_color >> 24) & 0xFF;
    const uint8_t g = (config.bg_color >> 16) & 0xFF;
    const uint8_t b = (config.bg_color >>  8) & 0xFF;
    const uint8_t a = (config.bg_color >>  0) & 0xFF;

    SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
    SDL_RenderClear(sdl.renderer);
}


// Update screen with any changes
void update_screen(const sdl_t sdl){
    SDL_RenderPresent(sdl.renderer);
}

void handle_input(chip8_t *chip8){
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type)
        {
        case SDL_QUIT:
            // Exit window. End program
            chip8->state = QUIT;
            return;

        case SDL_KEYDOWN:
            break;

        case SDL_KEYUP:
            break;

        default:
            break;
        }
    }
}

// Da main squeeze
int main(int argc, char **argv) {

    // Initialize emulator configuration/options
    config_t config = {0};
    if (!set_config_from_args(&config, argc, argv)) exit(EXIT_FAILURE);

    // Initialize SDL
    sdl_t sdl = {0};
    if (!init_sdl(&sdl, &config)) exit(EXIT_FAILURE);

    // Initialize Chip8 machine
    chip8_t chip8 = {0};
    if (!init_chip8(&chip8)) exit(EXIT_FAILURE);

    // Initial screen clear
    clear_screen(sdl, config)

    // Main emulator Loop
    // Chip8 has an instruction to conditionally clear the screen
    while (chip8.state != QUIT){
        // Handle input
        handle_input(&chip8);
        
        // if (chip8.state == PAUSED) continue

        // Get_time();
        // Emulate CHIP8 instructions
        // Get_time() elapsed since last Get_time();
        //
        // Delay for approximately 60HZ/60FPS, because the CRT TV was 60HZ back then
        // 16 because there was no float number, for 60HZ it is 16.67ms
        SDL_Delay(16);

        // Update window with changes
        update_screen(sdl);
    }

    // Final cleanup
    final_cleanup(sdl); 

    exit(EXIT_SUCCESS);
}
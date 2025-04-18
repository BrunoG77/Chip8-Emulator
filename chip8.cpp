// For smart pointers
#include <memory>

// For cout<< and cin>>
#include <iostream>
using namespace std;

/*to use sdl stuff (video graphics window, input events, audio, I/O, timers, threads etc)*/
#include <SDL.h>

// Defines a set of standard exceptions that both the library and programs can use to report common errors.
// I use the runtime_error
#include <stdexcept>


// Configuration structure to change speed, window, colors, etc
struct Config
{
    // The COSMAC 1802 processor is 8-bit, however I will use 32 bit numbers for resolution
    // 32 bit numbers for with and height
    uint32_t window_width = 64;     // Default Chip-8 resolution width
    uint32_t window_height = 32;    // Default Chip-8 resolution height
    uint32_t fg_color = 0xFFFFFFFF; // Foreground color WHITE in RGBA8888 format
    uint32_t bg_color = 0xFFFF00FF; // Background color YELLOW in RGBA8888 format
    // Amount to scale a CHIP8 pixel by e.g. 20x will be a 20x larger window
    uint32_t scale_factor = 20;     // Default resolution will now be 1280*640
};

// Emulator states
enum class EmulatorState {
    // state that emulator is running in
    QUIT,
    RUNNING,
    PAUSED,
};

// Chip8 Machine object
struct Chip8
{
    EmulatorState state;
};

// Custom deleter for SDL_Window using Operator OVERLOADING
// This will overload SDL_Window and add the SDL_DestroyWindow function to the SDL_window function
// The const qualifier means this operation doesn't modify the SDLWindowDeleter object itself.
struct SDLWindowDeleter {
    void operator()(SDL_Window* window) const {
        if (window){
            SDL_DestroyWindow(window);
        }
    }
};

// Use std::unique_ptr to create an alias to SDL window and its deletion
// This custom deleter is designed to work with std::unique_ptr, 
// allowing it to properly manage SDL_Window resources. When the unique_ptr is destroyed, 
// it will call this deleter to ensure the SDL_Window is correctly cleaned up
using SDLWindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;


// Do the same for the 2D renderer
struct SDLRendererDeleter {
    void operator()(SDL_Renderer* renderer) const {
        if (renderer){
            SDL_DestroyRenderer(renderer);
        }
    }
};

using SDLRendererPtr = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>;


// RAII class for managing SDL initialization and cleanup
// Manage resources via object lifetime (constructor acquires, destructor releases)
class SDLManager {
private:
    SDLWindowPtr window;
    SDLRendererPtr renderer;
    Config config;

public:
    explicit SDLManager(const Config& cfg) : config(cfg) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
            throw runtime_error(SDL_GetError());
        }

        // Automatically calls SDL_DestroyWindow(window) when the pointer is destroyed or reset.
        window.reset(SDL_CreateWindow(
            "Chip8 Emulator",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            config.window_width * config.scale_factor,
            config.window_height * config.scale_factor,
            0));

        if (!window) {
            throw runtime_error(SDL_GetError());
        }

        // Create Renderer
        renderer.reset(SDL_CreateRenderer(
            window.get(),   // Use window.get() to obtain the raw SDL_Window* pointer
            -1, // -1 to initialize the first one supporting the requested flags
            SDL_RENDERER_ACCELERATED    // The renderer uses hardware acceleration
        ));

        if (!renderer){
            throw runtime_error(SDL_GetError());
        }
    }

    // Clear screen / SDL Window to background color
    void clear_window(){
        const uint8_t r = (config.bg_color >> 24) & 0xFF;// slide x bit values to the side -> bitwise right shift operator
        const uint8_t g = (config.bg_color >> 16) & 0xFF;// to get the 8 bits in the 32bit RGBA
        const uint8_t b = (config.bg_color >>  8) & 0xFF;// then use 0xFF mask -> bitwise AND operator (&) with a mask
        const uint8_t a = (config.bg_color >>  0) & 0xFF;// so only the last 8 bits remain
    
        SDL_SetRenderDrawColor(renderer.get(), r, g, b, a);
        SDL_RenderClear(renderer.get());
    }

    void update_window() {
        SDL_RenderPresent(renderer.get());
    }
    
    // Function Destructor
    ~SDLManager() {
        // Cleanup handled automatically by unique_ptr's custom deleter
        SDL_Quit();
    }
};

// Initialize chip8 machine
bool init_chip8(Chip8 *chip8){
    chip8->state = EmulatorState::RUNNING; // Default machine state
    return true;    // Success
}

// Handle the input
void handle_input(Chip8 *chip8){
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type)
        {
        case SDL_QUIT:
            // Exit window. End program
            chip8->state = EmulatorState::QUIT;
            return;

        case SDL_KEYDOWN:
            switch(event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    chip8->state = EmulatorState::QUIT;
                    return;

                default:
                    break;
            }
            break;

        case SDL_KEYUP:
            break;

        default:
            break;
        }
    }
}

// Main code here
int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    try {
        // Get initial config
        Config config;

        // Initialize SDL with RAII
        SDLManager sdlManager(config);
        cout << "SDL Initialized" << endl;

        // Initial screen clear
        sdlManager.clear_window();

        // Initialize chip8
        Chip8 chip8 = {};
        init_chip8(&chip8);

        // Main emulator Loop
        // Chip8 has an instruction to conditionally clear the screen
        while (chip8.state != EmulatorState::QUIT){
            // Handle input
            handle_input(&chip8);

            // Get_time();
            // Emulate CHIP8 instructions
            // Get_time() elapsed since last Get_time();
            //
            // Delay for approximately 60HZ/60FPS, because the CRT TV was 60HZ back then
            // 16 because there was no float number, for 60HZ it is 16.67ms
            SDL_Delay(16);

            // Update window by presenting it
            sdlManager.update_window();
        }

        cout << "Quit" << endl;
        return EXIT_SUCCESS;

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
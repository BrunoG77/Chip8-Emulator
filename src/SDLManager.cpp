#include "SDLManager.hpp"

// Custom deleter for SDL_Window using Operator OVERLOADING
// This will overload SDL_Window and add the SDL_DestroyWindow function to the SDL_window function
// The const qualifier means this operation doesn't modify the SDLWindowDeleter object itself.
void SDLManager::SDLWindowDeleter::operator()(SDL_Window* window) const {
    if (window) SDL_DestroyWindow(window);
}

// Same for SDLRendererDeleter
void SDLManager::SDLRendererDeleter::operator()(SDL_Renderer* renderer) const {
    if (renderer) SDL_DestroyRenderer(renderer);
}

SDLManager::SDLManager(const Config& cfg) : config(cfg) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        throw std::runtime_error(SDL_GetError());
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
        throw std::runtime_error(SDL_GetError());
    }

    // Create Renderer
    renderer.reset(SDL_CreateRenderer(
        window.get(),   // Use window.get() to obtain the raw SDL_Window* pointer
        -1, // -1 to initialize the first one supporting the requested flags
        SDL_RENDERER_ACCELERATED    // The renderer uses hardware acceleration
    ));

    if (!renderer) {
        throw std::runtime_error(SDL_GetError());
    }
}

// Clear screen / SDL Window to background color
void SDLManager::clear_window() {
    const uint8_t r = (config.bg_color >> 24) & 0xFF;// slide x bit values to the side -> bitwise right shift operator
        const uint8_t g = (config.bg_color >> 16) & 0xFF;// to get the 8 bits in the 32bit RGBA
        const uint8_t b = (config.bg_color >>  8) & 0xFF;// then use 0xFF mask -> bitwise AND operator (&) with a mask
        const uint8_t a = (config.bg_color >>  0) & 0xFF;// so only the last 8 bits remain
    
    SDL_SetRenderDrawColor(renderer.get(), r, g, b, a);
    SDL_RenderClear(renderer.get());
}

void SDLManager::update_window() {
    SDL_RenderPresent(renderer.get());
}

// Function Destructor
SDLManager::~SDLManager() {
    // Cleanup handled automatically by unique_ptr's custom deleter
    SDL_Quit();
}
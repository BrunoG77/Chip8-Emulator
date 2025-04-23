#include "SDLManager.hpp"


namespace Chip8 {
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

    void SDLManager::update_window(const Config config, const Machine machine) {
        SDL_Rect rect = {.x = 0, 
            .y = 0, 
            .w = static_cast<int>(config.scale_factor), 
            .h = static_cast<int>(config.scale_factor)};

        // Grab color values to draw. Background and foreground rgba
        const uint8_t bg_r = (config.bg_color >> 24) & 0xFF;
        const uint8_t bg_g = (config.bg_color >> 16) & 0xFF;
        const uint8_t bg_b = (config.bg_color >>  8) & 0xFF;
        const uint8_t bg_a = (config.bg_color >>  0) & 0xFF;

        const uint8_t fg_r = (config.fg_color >> 24) & 0xFF;
        const uint8_t fg_g = (config.fg_color >> 16) & 0xFF;
        const uint8_t fg_b = (config.fg_color >>  8) & 0xFF;
        const uint8_t fg_a = (config.fg_color >>  0) & 0xFF;

        // Loop through all display pixels, draw a rectangle per pixel to the SDL window
        for (uint32_t i = 0; i < machine.display.size(); i++) {
            // Translate 1D index i value to 2D X/Y coordinates
            rect.x = (i % config.window_width) * config.scale_factor;
            rect.y = (i / config.window_width) * config.scale_factor;   // 64/64 is 1 | 128/64 is 2 etc. That is each 64 pixel width row

            if (machine.display[i]) {
                // The i pixel in the display is on, draw foreground color
                SDL_SetRenderDrawColor(renderer.get(), fg_r, fg_g, fg_b, fg_a);
                SDL_RenderFillRect(renderer.get(), &rect);

                // If user wants pixel outlines
                if (config.pixel_outlines) {
                    SDL_SetRenderDrawColor(renderer.get(), bg_r, bg_g, bg_b, bg_a);
                    SDL_RenderDrawRect(renderer.get(), &rect);
                }

            } else {
                // The pixel is off, draw the background color
                SDL_SetRenderDrawColor(renderer.get(), bg_r, bg_g, bg_b, bg_a);
                SDL_RenderFillRect(renderer.get(), &rect);
            }
        }

        SDL_RenderPresent(renderer.get());
    }

    // Function Destructor
    SDLManager::~SDLManager() {
        // Cleanup handled automatically by unique_ptr's custom deleter
        SDL_Quit();
    }
}
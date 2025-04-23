#pragma once
#include "Config.hpp"
#include "Chip8.hpp"
#include <SDL.h>
#include <memory>
#include <stdexcept>

namespace Chip8 {
    // RAII class for managing SDL initialization and cleanup
    // Manage resources via object lifetime (constructor acquires, destructor releases)
    class SDLManager {
    public:
        explicit SDLManager(const Config& cfg);
        ~SDLManager();
        
        void clear_window();
        void update_window(const Config config, const Machine machine);

        // Delete copy semantics
        SDLManager(const SDLManager&) = delete;
        SDLManager& operator=(const SDLManager&) = delete;

    private:
        // Custom deleter for SDL_Window using Operator OVERLOADING
        struct SDLWindowDeleter {
            void operator()(SDL_Window* window) const;
        };

        // Use std::unique_ptr to create an alias to SDL window and its deletion
        // This custom deleter is designed to work with std::unique_ptr, 
        // allowing it to properly manage SDL_Window resources. When the unique_ptr is destroyed, 
        // it will call this deleter to ensure the SDL_Window is correctly cleaned up
        using SDLWindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;
        
        // Do the same for the 2D renderer
        struct SDLRendererDeleter {
            void operator()(SDL_Renderer* renderer) const;
        };

        using SDLRendererPtr = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>;

        SDLWindowPtr window;
        SDLRendererPtr renderer;
        Config config;
    };
}
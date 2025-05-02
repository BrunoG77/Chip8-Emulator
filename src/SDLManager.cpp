#include "SDLManager.hpp"
#include <iostream>

namespace Chip8 {
    // Fill out stream/audio buffer with data
    // Audio callback as static member function
    static void audio_callback(void* userdata, uint8_t* stream, int len) {
        AudioState* state = static_cast<AudioState*>(userdata);

        const Config& config = *(state->config);

        int16_t* buffer = reinterpret_cast<int16_t*>(stream);

        const int samples = len / sizeof(int16_t);

        const int square_wave_period = config.audio_sample_rate / config.square_wave_freq;
        const int half_square_wave_period = square_wave_period / 2;

        // Filling out 2 bytes at a time(int16_t), len is in bytes, so divide by 2
        for (int i = 0; i < samples; i++) {
            if (state->playing_sound) {
                buffer[i] = ((state->sample_index++ / half_square_wave_period) % 2) ? 
                             config.volume : -config.volume;
            } else {
                buffer[i] = 0; // output silence
            }
        }
    }

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

    SDLManager::SDLManager(Config& cfg) : config(cfg) {
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

        // Initialize audio state on the heap
        audio_state = new AudioState{0, &config};

        SDL_AudioSpec want{};
        want.freq = config.audio_sample_rate;   // 44100hz "CD" quality
        want.format = AUDIO_S16LSB;             // signed 16 little indian cause its x86
        want.channels = 1;                      // Mono 1 channel
        want.samples = 512;
        want.callback = audio_callback;
        want.userdata = audio_state;           // To change volume data or other things, passed to audio callback
        // Safe as long as config outlives audio_dev

        audio_dev = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);

        if (audio_dev == 0) {
            delete audio_state;
            // If its not bigger than 0, has an error
            throw std::runtime_error("Failed to open Audio device" + std::string(SDL_GetError()) + "\n");
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
        // Initialize rectangle
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
            // (i % config.window_width) -> If window_width is 64, pixels 0–63 are in row 0, 64–127 in row 1, etc.
            rect.x = (i % config.window_width) * config.scale_factor;   // If i = 70: 70 % 64 = 6 -> column 6.
            // Get each 64 pixel width row
            rect.y = (i / config.window_width) * config.scale_factor;   // If i = 70: 70 / 64 = 1 → row 1

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

    void SDLManager::handle_audio(const Machine& machine) {
        if (machine.sound_timer > 0 && !audio_state->playing_sound) {
            SDL_PauseAudioDevice(audio_dev, 0); // Play
            audio_state->playing_sound = true;

        } else if (machine.sound_timer == 0 && audio_state->playing_sound) {
            SDL_PauseAudioDevice(audio_dev, 1); // Pause
            audio_state->playing_sound = false;
        }
        std::cout << "Sound Timer: " << machine.sound_timer << ", Playing: " 
        << audio_state->playing_sound << std::endl;
    }

    // Function Destructor
    SDLManager::~SDLManager() {
        // Close audio device before quitting
        if (audio_dev != 0) {
            SDL_CloseAudioDevice(audio_dev);
        }
        delete audio_state;

        // Cleanup handled automatically by unique_ptr's custom deleter
        SDL_Quit();
    }
}
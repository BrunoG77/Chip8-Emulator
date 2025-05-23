#pragma once
#include <cstdint>

// Configuration structure to change speed, window, colors, etc
struct Config
{
    // The COSMAC 1802 processor is 8-bit, however I will use 32 bit numbers for resolution
    // 32 bit numbers for with and height
    static constexpr uint32_t window_width = 64;     // Default Chip-8 resolution width
    static constexpr uint32_t window_height = 32;    // Default Chip-8 resolution height
    uint32_t fg_color = 0xFFFFFFFF; // Foreground color WHITE in RGBA8888 format
    uint32_t bg_color = 0x000000FF; // Background color BLACK in RGBA8888 format
    // Amount to scale a CHIP8 pixel by e.g. 20x will be a 20x larger window
    uint32_t scale_factor = 20;     // Default resolution will now be 1280*640
    bool pixel_outlines = true;     // Draw pixel outlines yes/no
    uint32_t ints_per_second = 700;// CHIP8 CPU "clock rates" or hertz
    uint32_t square_wave_freq = 440;       // Frequency of square wave sound e.g. 440hz for middle A
    uint32_t audio_sample_rate = 44100;     // "CD" quality, 44100 hz
    // int16, cause its little indian or negative volume
    int16_t volume = 3000;                 // How loud or not is the sound
    // INT16_MAX would be max volume
};

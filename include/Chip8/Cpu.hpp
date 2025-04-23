#pragma once
#include "Chip8.hpp"

namespace Chip8 {
    void emulate_instruction(Machine& machine, const Config config);
}
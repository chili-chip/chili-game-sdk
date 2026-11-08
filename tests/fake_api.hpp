#pragma once

// Stands in for the 32blit firmware in the tests: buttons, a clock, audio
// channels and an in-memory file system for saves.

#include "32blit.hpp"

#include <map>
#include <string>

namespace fake {

// Clears buttons, files, channels and the clock. The runner calls it before
// each test.
void reset();

// Sets the held buttons for the next frame and works out `pressed` and
// `released` from the previous frame, like the firmware does.
void hold(uint32_t buttons);

// Files written so far, by path.
std::map<std::string, std::string> &files();

}

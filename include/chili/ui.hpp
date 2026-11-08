#pragma once

#include "32blit.hpp"

#include <cstdint>

// Things the menu and screens share: the colours they draw with by default,
// and one hook for their sounds.
namespace chili {

// What a menu or screen did, so the game can play a sound for it.
enum class UiSound : uint8_t {
  move,    // cursor moved
  change,  // a value changed (toggle, choice, letter)
  select,  // A on an action, or confirm
  back,    // B, or leaving a screen
};

// Called for every UiSound. Set it once in init(); nullptr plays nothing.
extern void (*ui_sound)(UiSound sound);

// Calls ui_sound if one is set.
void play_ui_sound(UiSound sound);

// Default colours for the menu, the screens and the panel helper. Change the
// fields in init() to restyle every widget at once.
struct Theme {
  blit::Pen text{240, 240, 236};
  blit::Pen dim{140, 140, 150};
  blit::Pen accent{255, 140, 40};
  blit::Pen highlight{198, 40, 40, 170};
  blit::Pen panel{16, 16, 24, 235};
  blit::Pen border{198, 40, 40};
  blit::Pen gold{255, 210, 60};
  blit::Pen shadow{0, 0, 0, 200};
};

extern Theme theme;

}

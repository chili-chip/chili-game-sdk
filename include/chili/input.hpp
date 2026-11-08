#pragma once

#include "32blit.hpp"

#include <cstdint>

// Input helpers. Remember: blit::pressed() means "held". A button pressed
// this frame is in blit::buttons.pressed.
namespace chili {

// True on the frame the button went down.
inline bool just_pressed(uint32_t button) {
  return blit::buttons.pressed & button;
}

// Up/down menu navigation with wrap-around. Returns true when the index moved.
// `pressed` defaults to this frame's presses; pass your own to filter them.
bool menu_nav(int &index, int count, uint32_t pressed = blit::buttons.pressed);

// Auto-repeat for held buttons, like a keyboard: fires once on press, then
// after `delay_ms` keeps firing every `rate_ms` while held. Each button is
// timed on its own. Fires at most once per frame.
class Repeat {
public:
  explicit Repeat(uint32_t delay_ms = 250, uint32_t rate_ms = 80);

  // Call once per update with the held buttons and the frame time in ms.
  // Returns the buttons that fire this frame.
  uint32_t update(uint32_t held, uint32_t dt);
  // Same, reading blit::buttons.
  uint32_t update(uint32_t dt) { return update(blit::buttons.state, dt); }

  // Whether the button fired on the last update.
  bool fired(uint32_t button) const { return fired_ & button; }

  // Forget held buttons, for example when a scene changes.
  void reset();

  uint32_t delay_ms;
  uint32_t rate_ms;

private:
  static constexpr int kButtons = 16;
  uint32_t held_ = 0;
  uint32_t fired_ = 0;
  uint32_t held_ms_[kButtons] = {};
  uint32_t next_ms_[kButtons] = {};
};

}

#include "chili/input.hpp"

using namespace blit;

namespace chili {

bool menu_nav(int &index, int count, uint32_t pressed) {
  if(count <= 0) return false;
  if(pressed & Button::DPAD_UP) {
    index = (index + count - 1) % count;
    return true;
  }
  if(pressed & Button::DPAD_DOWN) {
    index = (index + 1) % count;
    return true;
  }
  return false;
}

Repeat::Repeat(uint32_t delay_ms, uint32_t rate_ms) : delay_ms(delay_ms), rate_ms(rate_ms) {}

uint32_t Repeat::update(uint32_t held, uint32_t dt) {
  fired_ = 0;
  for(int i = 0; i < kButtons; i++) {
    uint32_t bit = 1u << i;
    if(!(held & bit)) continue;

    if(!(held_ & bit)) {
      // just pressed
      held_ms_[i] = 0;
      next_ms_[i] = delay_ms;
      fired_ |= bit;
      continue;
    }

    held_ms_[i] += dt;
    if(held_ms_[i] >= next_ms_[i]) {
      fired_ |= bit;
      // after a long frame, skip the missed repeats instead of bunching them
      while(next_ms_[i] <= held_ms_[i]) next_ms_[i] += rate_ms ? rate_ms : 1;
    }
  }
  held_ = held;
  return fired_;
}

void Repeat::reset() {
  held_ = 0;
  fired_ = 0;
}

}

#include "chili/scene.hpp"

#include "chili/fx.hpp"

using namespace blit;

namespace chili {

void Scenes::enter(Scene &next) {
  scene = &next;
  in_scene = 0;
  stale = buttons.state;
  next.enter();
}

void Scenes::go(Scene &next, uint32_t fade) {
  if(fade == 0 || !scene) {
    phase = Phase::none;
    pending = nullptr;
    enter(next);
    return;
  }
  if(phase != Phase::out) {
    phase = Phase::out;
    fade_t = 0;
  }
  fade_ms = fade;
  pending = &next;
}

void Scenes::update(uint32_t time) {
  frame_ms = started ? time - last_time : 0;
  last_time = time;
  started = true;
  stale &= buttons.state;

  if(phase == Phase::out) {
    fade_t += frame_ms;
    if(fade_t < fade_ms) return;
    phase = Phase::in;
    fade_t = 0;
    Scene *next = pending;
    pending = nullptr;
    enter(*next);
    return;
  }

  if(phase == Phase::in) {
    fade_t += frame_ms;
    if(fade_t >= fade_ms) phase = Phase::none;
  }

  in_scene += frame_ms;
  if(scene) scene->update(frame_ms);
}

void Scenes::render(uint32_t time) {
  if(!scene) return;
  scene->render(time);
  uint8_t alpha = fade_alpha();
  if(alpha) fx::dim(alpha, fade_color);
}

uint8_t Scenes::fade_alpha() const {
  if(phase == Phase::none || fade_ms == 0) return 0;
  uint32_t t = fade_t < fade_ms ? fade_t : fade_ms;
  if(phase == Phase::in) t = fade_ms - t;
  return uint8_t(t * 255 / fade_ms);
}

uint32_t Scenes::pressed() const {
  if(fading() || in_scene < guard_ms) return 0;
  return buttons.pressed & ~stale;
}

uint32_t Scenes::held() const {
  return buttons.state & ~stale;
}

}

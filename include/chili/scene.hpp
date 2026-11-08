#pragma once

#include "32blit.hpp"

#include <cstdint>

namespace chili {

// One screen of the game: title, play, pause, results... Override what you
// need. enter() runs each time the scene starts.
class Scene {
public:
  virtual ~Scene() = default;
  virtual void enter() {}
  virtual void update(uint32_t dt) {}
  virtual void render(uint32_t time) {}
};

// Runs the current scene and switches between scenes, with an optional fade
// through black. Call update() and render() from blit's update() and render().
//
// Scenes are your own objects; Scenes only points at them, so keep them alive
// (globals are fine).
class Scenes {
public:
  // Switches to `next`. With fade_ms, the current scene freezes and fades to
  // black over fade_ms, then `next` starts and fades in over fade_ms.
  void go(Scene &next, uint32_t fade_ms = 0);

  void update(uint32_t time);
  void render(uint32_t time);

  Scene *current() const { return scene; }
  bool is(const Scene &s) const { return scene == &s; }

  // Milliseconds since the current scene started.
  uint32_t time_in_scene() const { return in_scene; }
  // Milliseconds since the last update.
  uint32_t dt() const { return frame_ms; }
  bool fading() const { return phase != Phase::none; }
  // How dark the fade is right now: 0 (clear) to 255 (black).
  uint8_t fade_alpha() const;

  // This frame's presses, for scenes to read instead of blit::buttons.pressed.
  // Leaves out buttons held since the last switch (until released), and is 0
  // while fading or for guard_ms after a switch.
  uint32_t pressed() const;
  // Held buttons, leaving out the ones held since the last switch.
  uint32_t held() const;

  // Ignore presses this long after each switch, so a player mashing buttons
  // does not skip the next screen.
  uint32_t guard_ms = 0;
  blit::Pen fade_color = blit::Pen(0, 0, 0);

private:
  enum class Phase : uint8_t { none, out, in };

  void enter(Scene &next);

  Scene *scene = nullptr;
  Scene *pending = nullptr;
  Phase phase = Phase::none;
  uint32_t fade_ms = 0;
  uint32_t fade_t = 0;

  bool started = false;
  uint32_t last_time = 0;
  uint32_t frame_ms = 0;
  uint32_t in_scene = 0;
  uint32_t stale = 0;  // buttons held since the last switch
};

}

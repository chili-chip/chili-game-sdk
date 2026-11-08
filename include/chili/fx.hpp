#pragma once

#include "32blit.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

// Screen effects and text helpers. Everything draws to blit::screen.
namespace chili::fx {

// ---------------------------------------------------------------------------
// Text and shapes

// Text with a 1 px drop shadow (theme.shadow).
void text(std::string_view s, blit::Point p, blit::Pen color,
          blit::TextAlign align = blit::TextAlign::top_left, bool shadow = true,
          const blit::Font &font = blit::minimal_font);

// Text scaled up by a whole number, centred on `center`, with a shadow.
// One line, up to 128 px wide before scaling.
void big_text(std::string_view s, blit::Point center, int scale, blit::Pen color,
              blit::Pen shadow = blit::Pen(0, 0, 0, 220));

// A filled box with a 1 px border and cut corners. Defaults to the theme.
void panel(blit::Rect r);
void panel(blit::Rect r, blit::Pen fill, blit::Pen border);

// Covers the whole screen with `color` at `alpha` (0 = clear, 255 = solid).
// Use it to dim the game behind a pause menu, or to fade.
void dim(uint8_t alpha, blit::Pen color = blit::Pen(0, 0, 0));

// ---------------------------------------------------------------------------
// Particles

// How a burst throws its particles.
struct Burst {
  float speed_min = 0.02f;    // px per ms
  float speed_max = 0.07f;
  float lift = 0.03f;         // extra upward speed, px per ms
  uint16_t life_min = 300;    // ms
  uint16_t life_max = 600;
  // Half the particles take this colour instead. Alpha 0 turns it off.
  blit::Pen sparkle = blit::Pen(255, 255, 220);
};

// A fixed pool of pixel particles that fade out and fall.
class Particles {
public:
  explicit Particles(size_t capacity = 48);

  // Throws up to `count` particles from `at`. When the pool is full, the
  // extra particles are dropped.
  void burst(blit::Point at, blit::Pen color, int count, const Burst &how = Burst());

  void update(uint32_t dt);
  // Draws every live particle, moved by `offset` (for screen shake).
  void render(blit::Point offset = blit::Point(0, 0)) const;
  void clear();

  int active() const;

  float gravity = 0.00015f;  // px per ms per ms

private:
  struct Particle {
    float x, y, vx, vy;
    int16_t life, max_life;
    blit::Pen color;
  };
  std::vector<Particle> pool;
};

// ---------------------------------------------------------------------------
// Floating text

// Short texts that rise and fade, like "+100" score popups. When all slots
// are busy, the oldest one is replaced.
class FloatingText {
public:
  explicit FloatingText(size_t slots = 6);

  // Shows `s` (up to 11 characters) centred on `at`.
  void add(blit::Point at, std::string_view s, blit::Pen color);

  void update(uint32_t dt);
  void render(blit::Point offset = blit::Point(0, 0)) const;
  void clear();

  int active() const;

  uint16_t life_ms = 700;
  float rise = 0.02f;         // px per ms
  uint16_t fade_ms = 300;     // fades out over the last fade_ms

private:
  struct Slot {
    float x, y;
    int16_t life;
    char text[12];
    blit::Pen color;
  };
  std::vector<Slot> slots;
};

// ---------------------------------------------------------------------------
// Screen shake

// A random offset that shrinks to nothing. Add offset() to what you draw.
class Shake {
public:
  // Shakes for `duration_ms`. The offset is up to remaining / ms_per_px + 1
  // pixels, so it calms down as it ends.
  void start(uint32_t duration_ms, uint32_t ms_per_px = 120);
  void update(uint32_t dt);
  void stop() { remaining = 0; }

  bool active() const { return remaining > 0; }
  // A new random offset on each call while active, else (0, 0).
  blit::Point offset() const;

private:
  uint32_t remaining = 0;
  uint32_t step = 120;
};

}

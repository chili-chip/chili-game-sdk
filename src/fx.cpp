#include "chili/fx.hpp"

#include "chili/ui.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace blit;

namespace chili::fx {

void text(std::string_view s, Point p, Pen color, TextAlign align, bool shadow, const Font &font) {
  if(shadow) {
    screen.pen = theme.shadow;
    screen.text(s, font, p + Point(1, 1), true, align);
  }
  screen.pen = color;
  screen.text(s, font, p, true, align);
}

void big_text(std::string_view s, Point center, int scale, Pen color, Pen shadow) {
  static uint8_t buf[128 * 8 * 4];
  Surface tmp(buf, PixelFormat::RGBA, Size(128, 8));

  Size sz = screen.measure_text(s, minimal_font, true);
  int w = std::min(int(sz.w), 128);
  Rect src(0, 0, w, 8);
  Rect dst(center.x - w * scale / 2, center.y - 4 * scale, w * scale, 8 * scale);

  for(int pass = 0; pass < 2; pass++) {
    memset(buf, 0, sizeof(buf));
    tmp.pen = pass == 0 ? shadow : color;
    tmp.text(s, minimal_font, Point(0, 0), true);
    Rect d = dst;
    if(pass == 0) {
      d.x += scale / 2 + 1;
      d.y += scale / 2 + 1;
    }
    screen.stretch_blit(&tmp, src, d);
  }
}

void panel(Rect r) {
  panel(r, theme.panel, theme.border);
}

void panel(Rect r, Pen fill, Pen border) {
  screen.pen = fill;
  screen.rectangle(r);
  screen.pen = border;
  screen.rectangle(Rect(r.x + 1, r.y, r.w - 2, 1));
  screen.rectangle(Rect(r.x + 1, r.y + r.h - 1, r.w - 2, 1));
  screen.rectangle(Rect(r.x, r.y + 1, 1, r.h - 2));
  screen.rectangle(Rect(r.x + r.w - 1, r.y + 1, 1, r.h - 2));
}

void dim(uint8_t alpha, Pen color) {
  color.a = alpha;
  screen.pen = color;
  screen.rectangle(Rect(Point(0, 0), screen.bounds));
}

// ---------------------------------------------------------------------------

Particles::Particles(size_t capacity) : pool(capacity, Particle{0, 0, 0, 0, 0, 1, Pen()}) {}

void Particles::burst(Point at, Pen color, int count, const Burst &how) {
  for(auto &pt : pool) {
    if(count <= 0) break;
    if(pt.life > 0) continue;
    float ang = (blit::random() % 628) / 100.0f;
    float spd = how.speed_min + (blit::random() % 100) / 100.0f * (how.speed_max - how.speed_min);
    pt.x = at.x;
    pt.y = at.y;
    pt.vx = cosf(ang) * spd;
    pt.vy = sinf(ang) * spd - how.lift;
    int span = std::max(1, how.life_max - how.life_min);
    pt.max_life = pt.life = int16_t(how.life_min + blit::random() % span);
    pt.color = ((blit::random() & 1) && how.sparkle.a) ? how.sparkle : color;
    count--;
  }
}

void Particles::update(uint32_t dt) {
  for(auto &pt : pool) {
    if(pt.life <= 0) continue;
    pt.life -= int16_t(std::min<uint32_t>(dt, 0x7fff));
    pt.x += pt.vx * dt;
    pt.y += pt.vy * dt;
    pt.vy += gravity * dt;
  }
}

void Particles::render(Point offset) const {
  for(auto &pt : pool) {
    if(pt.life <= 0) continue;
    Pen c = pt.color;
    c.a = uint8_t(c.a * pt.life / pt.max_life);
    screen.pen = c;
    screen.pixel(Point(int(pt.x), int(pt.y)) + offset);
  }
}

void Particles::clear() {
  for(auto &pt : pool) pt.life = 0;
}

int Particles::active() const {
  return int(std::count_if(pool.begin(), pool.end(), [](const Particle &p) { return p.life > 0; }));
}

// ---------------------------------------------------------------------------

FloatingText::FloatingText(size_t count) : slots(count, Slot{0, 0, 0, {0}, Pen()}) {}

void FloatingText::add(Point at, std::string_view s, Pen color) {
  if(slots.empty()) return;
  Slot *slot = &slots[0];
  for(auto &sl : slots) {
    if(sl.life <= 0) {
      slot = &sl;
      break;
    }
    if(sl.life < slot->life) slot = &sl;
  }
  slot->x = at.x;
  slot->y = at.y;
  slot->life = int16_t(life_ms);
  size_t n = std::min(s.size(), sizeof(slot->text) - 1);
  memcpy(slot->text, s.data(), n);
  slot->text[n] = 0;
  slot->color = color;
}

void FloatingText::update(uint32_t dt) {
  for(auto &sl : slots) {
    if(sl.life <= 0) continue;
    sl.life -= int16_t(std::min<uint32_t>(dt, 0x7fff));
    sl.y -= rise * dt;
  }
}

void FloatingText::render(Point offset) const {
  for(auto &sl : slots) {
    if(sl.life <= 0) continue;
    Pen c = sl.color;
    int fade = std::max<int>(1, fade_ms);
    c.a = uint8_t(std::min(255, sl.life * 255 / fade) * c.a / 255);
    Point at = Point(int(sl.x), int(sl.y)) + offset;
    screen.pen = Pen(0, 0, 0, c.a);
    screen.text(sl.text, minimal_font, at + Point(1, 1), true, TextAlign::center_center);
    screen.pen = c;
    screen.text(sl.text, minimal_font, at, true, TextAlign::center_center);
  }
}

void FloatingText::clear() {
  for(auto &sl : slots) sl.life = 0;
}

int FloatingText::active() const {
  return int(std::count_if(slots.begin(), slots.end(), [](const Slot &s) { return s.life > 0; }));
}

// ---------------------------------------------------------------------------

void Shake::start(uint32_t duration_ms, uint32_t ms_per_px) {
  remaining = duration_ms;
  step = ms_per_px ? ms_per_px : 1;
}

void Shake::update(uint32_t dt) {
  remaining = dt >= remaining ? 0 : remaining - dt;
}

Point Shake::offset() const {
  if(!remaining) return Point(0, 0);
  int mag = int(remaining / step) + 1;
  int span = mag * 2 + 1;
  int x = int(blit::random() % span) - mag;
  int y = int(blit::random() % span) - mag;
  return Point(x, y);
}

}

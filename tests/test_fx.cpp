#include "check.hpp"
#include "fake_api.hpp"

#include "chili/fx.hpp"

#include <cstdlib>

using namespace blit;

TEST(burst_is_capped_by_the_pool) {
  chili::fx::Particles p(10);
  p.burst(Point(64, 64), Pen(255, 0, 0), 6);
  CHECK_EQ(p.active(), 6);
  p.burst(Point(64, 64), Pen(255, 0, 0), 6);
  CHECK_EQ(p.active(), 10);
}

TEST(particles_die_after_their_life) {
  chili::fx::Particles p(10);
  chili::fx::Burst how;
  how.life_min = 100;
  how.life_max = 101;
  p.burst(Point(64, 64), Pen(255, 0, 0), 4, how);
  p.update(50);
  CHECK_EQ(p.active(), 4);
  p.update(60);
  CHECK_EQ(p.active(), 0);
}

TEST(particles_clear) {
  chili::fx::Particles p;
  p.burst(Point(0, 0), Pen(), 5);
  p.clear();
  CHECK_EQ(p.active(), 0);
}

TEST(floating_text_replaces_the_oldest_when_full) {
  chili::fx::FloatingText t(2);
  t.add(Point(10, 10), "+1", Pen());
  t.update(100);
  t.add(Point(20, 10), "+2", Pen());
  t.add(Point(30, 10), "+3", Pen());
  CHECK_EQ(t.active(), 2);
  t.update(650);  // "+1" would be gone by now, but it was replaced anyway
  CHECK_EQ(t.active(), 2);
  t.update(100);
  CHECK_EQ(t.active(), 0);
}

TEST(shake_shrinks_and_stops) {
  chili::fx::Shake s;
  CHECK(s.offset() == Point(0, 0));
  s.start(450, 120);
  CHECK(s.active());
  for(int i = 0; i < 20; i++) {
    Point o = s.offset();
    CHECK(std::abs(o.x) <= 4 && std::abs(o.y) <= 4);
  }
  s.update(400);
  for(int i = 0; i < 20; i++) {
    Point o = s.offset();
    CHECK(std::abs(o.x) <= 1 && std::abs(o.y) <= 1);
  }
  s.update(100);
  CHECK(!s.active());
  CHECK(s.offset() == Point(0, 0));
}

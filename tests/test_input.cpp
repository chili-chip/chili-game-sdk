#include "check.hpp"
#include "fake_api.hpp"

#include "chili/input.hpp"

using namespace blit;

TEST(just_pressed_reads_pressed_not_held) {
  fake::hold(Button::A);
  CHECK(chili::just_pressed(Button::A));
  fake::hold(Button::A);  // still held next frame
  CHECK(!chili::just_pressed(Button::A));
  CHECK(blit::pressed(Button::A));
}

TEST(menu_nav_moves_down_and_wraps) {
  int i = 2;
  CHECK(chili::menu_nav(i, 3, Button::DPAD_DOWN));
  CHECK_EQ(i, 0);
  CHECK(chili::menu_nav(i, 3, Button::DPAD_DOWN));
  CHECK_EQ(i, 1);
}

TEST(menu_nav_moves_up_and_wraps) {
  int i = 0;
  CHECK(chili::menu_nav(i, 3, Button::DPAD_UP));
  CHECK_EQ(i, 2);
}

TEST(menu_nav_ignores_other_buttons) {
  int i = 1;
  CHECK(!chili::menu_nav(i, 3, Button::A | Button::DPAD_LEFT));
  CHECK_EQ(i, 1);
}

TEST(menu_nav_reads_buttons_by_default) {
  int i = 0;
  fake::hold(Button::DPAD_DOWN);
  CHECK(chili::menu_nav(i, 4));
  CHECK_EQ(i, 1);
}

TEST(repeat_fires_on_press) {
  chili::Repeat r(200, 50);
  r.update(Button::DPAD_LEFT, 10);
  CHECK(r.fired(Button::DPAD_LEFT));
  CHECK(!r.fired(Button::DPAD_RIGHT));
}

TEST(repeat_waits_for_delay) {
  chili::Repeat r(200, 50);
  r.update(Button::DPAD_LEFT, 10);
  r.update(Button::DPAD_LEFT, 100);
  CHECK(!r.fired(Button::DPAD_LEFT));
  r.update(Button::DPAD_LEFT, 99);  // 199 ms held
  CHECK(!r.fired(Button::DPAD_LEFT));
  r.update(Button::DPAD_LEFT, 1);   // 200 ms held
  CHECK(r.fired(Button::DPAD_LEFT));
}

TEST(repeat_fires_at_rate_after_delay) {
  chili::Repeat r(200, 50);
  r.update(Button::DPAD_DOWN, 0);
  r.update(Button::DPAD_DOWN, 200);
  CHECK(r.fired(Button::DPAD_DOWN));
  r.update(Button::DPAD_DOWN, 49);
  CHECK(!r.fired(Button::DPAD_DOWN));
  r.update(Button::DPAD_DOWN, 1);
  CHECK(r.fired(Button::DPAD_DOWN));
  r.update(Button::DPAD_DOWN, 50);
  CHECK(r.fired(Button::DPAD_DOWN));
}

TEST(repeat_fires_once_per_frame_after_a_stall) {
  chili::Repeat r(200, 50);
  r.update(Button::DPAD_DOWN, 0);
  CHECK_EQ(r.update(Button::DPAD_DOWN, 1000), uint32_t(Button::DPAD_DOWN));
  r.update(Button::DPAD_DOWN, 10);
  CHECK(!r.fired(Button::DPAD_DOWN));
}

TEST(repeat_restarts_after_release) {
  chili::Repeat r(200, 50);
  r.update(Button::DPAD_LEFT, 0);
  r.update(Button::DPAD_LEFT, 150);
  r.update(0, 10);
  CHECK(!r.fired(Button::DPAD_LEFT));
  r.update(Button::DPAD_LEFT, 10);
  CHECK(r.fired(Button::DPAD_LEFT));
  r.update(Button::DPAD_LEFT, 150);  // the delay starts again
  CHECK(!r.fired(Button::DPAD_LEFT));
}

TEST(repeat_tracks_buttons_separately) {
  chili::Repeat r(200, 50);
  r.update(Button::DPAD_LEFT, 0);
  r.update(Button::DPAD_LEFT, 150);
  r.update(Button::DPAD_LEFT | Button::DPAD_DOWN, 10);
  CHECK(!r.fired(Button::DPAD_LEFT));
  CHECK(r.fired(Button::DPAD_DOWN));
  r.update(Button::DPAD_LEFT | Button::DPAD_DOWN, 40);  // left held 200 ms
  CHECK(r.fired(Button::DPAD_LEFT));
  CHECK(!r.fired(Button::DPAD_DOWN));
}

TEST(repeat_reset_forgets_held_buttons) {
  chili::Repeat r(200, 50);
  r.update(Button::DPAD_LEFT, 0);
  r.reset();
  CHECK(!r.fired(Button::DPAD_LEFT));
  r.update(Button::DPAD_LEFT, 10);
  CHECK(r.fired(Button::DPAD_LEFT));
}

TEST(repeat_reads_buttons_by_default) {
  chili::Repeat r;
  fake::hold(Button::DPAD_RIGHT);
  r.update(10);
  CHECK(r.fired(Button::DPAD_RIGHT));
}

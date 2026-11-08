#include "check.hpp"
#include "fake_api.hpp"

#include "chili/scene.hpp"

#include <vector>

using namespace blit;

struct Probe : chili::Scene {
  int entered = 0;
  int updates = 0;
  int renders = 0;
  uint32_t last_dt = 0;
  void enter() override { entered++; }
  void update(uint32_t dt) override {
    updates++;
    last_dt = dt;
  }
  void render(uint32_t) override { renders++; }
};

TEST(go_enters_the_scene_right_away) {
  chili::Scenes scenes;
  Probe a;
  CHECK(scenes.current() == nullptr);
  scenes.go(a);
  CHECK(scenes.current() == &a);
  CHECK(scenes.is(a));
  CHECK_EQ(a.entered, 1);
}

TEST(update_passes_frame_time_and_counts_time_in_scene) {
  chili::Scenes scenes;
  Probe a;
  scenes.go(a);
  scenes.update(1000);  // first frame: no time has passed yet
  CHECK_EQ(a.last_dt, uint32_t(0));
  scenes.update(1010);
  CHECK_EQ(a.last_dt, uint32_t(10));
  scenes.update(1030);
  CHECK_EQ(a.updates, 3);
  CHECK_EQ(scenes.time_in_scene(), uint32_t(30));
  CHECK_EQ(scenes.dt(), uint32_t(20));
}

TEST(time_in_scene_restarts_on_go) {
  chili::Scenes scenes;
  Probe a, b;
  scenes.go(a);
  scenes.update(0);
  scenes.update(500);
  scenes.go(b);
  CHECK_EQ(scenes.time_in_scene(), uint32_t(0));
  scenes.update(510);
  CHECK_EQ(scenes.time_in_scene(), uint32_t(10));
  CHECK_EQ(a.updates, 2);
  CHECK_EQ(b.updates, 1);
}

TEST(render_draws_the_current_scene) {
  chili::Scenes scenes;
  Probe a;
  scenes.render(0);  // nothing to draw yet
  scenes.go(a);
  scenes.render(0);
  CHECK_EQ(a.renders, 1);
}

TEST(button_held_through_a_switch_is_ignored_until_released) {
  chili::Scenes scenes;
  Probe a, b;
  scenes.go(a);
  fake::hold(Button::A | Button::DPAD_DOWN);
  scenes.update(0);
  CHECK_EQ(scenes.pressed(), uint32_t(Button::A | Button::DPAD_DOWN));
  scenes.go(b);  // A pressed on this frame opened scene b
  CHECK_EQ(scenes.pressed(), uint32_t(0));
  CHECK_EQ(scenes.held(), uint32_t(0));

  fake::hold(Button::DPAD_DOWN);  // A released, down still held
  scenes.update(10);
  CHECK_EQ(scenes.held(), uint32_t(0));

  fake::hold(Button::DPAD_DOWN | Button::A);  // A pressed again
  scenes.update(20);
  CHECK_EQ(scenes.pressed(), uint32_t(Button::A));
  CHECK_EQ(scenes.held(), uint32_t(Button::A));

  fake::hold(0);
  scenes.update(30);
  fake::hold(Button::DPAD_DOWN);  // down released and pressed again
  scenes.update(40);
  CHECK_EQ(scenes.held(), uint32_t(Button::DPAD_DOWN));
}

TEST(guard_blocks_presses_right_after_a_switch) {
  chili::Scenes scenes;
  scenes.guard_ms = 400;
  Probe a;
  scenes.go(a);
  scenes.update(0);
  fake::hold(Button::A);
  scenes.update(399);
  CHECK_EQ(scenes.pressed(), uint32_t(0));
  fake::hold(0);
  scenes.update(400);
  fake::hold(Button::A);
  scenes.update(410);
  CHECK_EQ(scenes.pressed(), uint32_t(Button::A));
}

TEST(fade_switches_halfway_and_blocks_input) {
  chili::Scenes scenes;
  Probe a, b;
  scenes.go(a);
  scenes.update(0);

  scenes.go(b, 200);
  CHECK(scenes.fading());
  CHECK(scenes.is(a));
  CHECK_EQ(b.entered, 0);
  CHECK_EQ(scenes.fade_alpha(), 0);

  scenes.update(100);  // half way to black: a is frozen
  CHECK_EQ(a.updates, 1);
  CHECK_EQ(scenes.fade_alpha(), 127);
  fake::hold(Button::A);
  CHECK_EQ(scenes.pressed(), uint32_t(0));

  fake::hold(0);
  scenes.update(200);  // black: switch to b
  CHECK(scenes.is(b));
  CHECK_EQ(b.entered, 1);
  CHECK_EQ(scenes.fade_alpha(), 255);

  scenes.update(300);  // fading in: b runs
  CHECK_EQ(b.updates, 1);
  CHECK_EQ(scenes.fade_alpha(), 127);

  scenes.update(400);
  CHECK(!scenes.fading());
  CHECK_EQ(scenes.fade_alpha(), 0);
  CHECK_EQ(scenes.time_in_scene(), uint32_t(200));
}

TEST(go_during_a_fade_out_changes_the_target) {
  chili::Scenes scenes;
  Probe a, b, c;
  scenes.go(a);
  scenes.update(0);
  scenes.go(b, 100);
  scenes.update(50);
  scenes.go(c, 100);
  scenes.update(100);
  CHECK(scenes.is(c));
  CHECK_EQ(b.entered, 0);
  CHECK_EQ(c.entered, 1);
}

#include "rpg_world.hpp"

using namespace blit;
using namespace chili::rpg;

TEST(starts_in_the_start_room) {
  World w;
  CHECK(play::start(w));
  CHECK(w.room().id == "home");
  CHECK_EQ(w.player.x, 1);
  CHECK_EQ(w.player.y, 1);
  CHECK_EQ(w.actors.size(), 3);
}

TEST(walls_and_water_block_walking) {
  World w;
  CHECK(play::start(w));
  CHECK(w.blocked(0, 0));
  CHECK(w.blocked(5, 3));      // water
  CHECK(!w.blocked(5, 3, true));  // but you can see across it
  CHECK(w.blocked(-1, 4));     // off the map
  CHECK(!w.blocked(2, 2));
}

TEST(a_small_room_does_not_scroll) {
  World w;
  CHECK(play::start(w));
  CHECK(w.camera() == Point(0, 0));
}

TEST(the_camera_follows_and_stops_at_edges) {
  World w;
  CHECK(play::start(w));
  w.warp(1, 1, 1, false);
  CHECK(w.camera() == Point(0, 0));
  play::place(w, 20, 8);
  CHECK(w.camera().x == 20 * 8 + 4 - 64);
  play::place(w, 30, 8);
  CHECK(w.camera().x == 32 * 8 - 128);
}

TEST(a_warp_tile_moves_to_the_other_room_after_a_fade) {
  World w;
  CHECK(play::start(w));
  play::place(w, 14, 8);
  play::press(w, Button::DPAD_RIGHT, 600);
  CHECK(w.room().id == "cave");
  CHECK_EQ(w.player.x, 1);
  CHECK_EQ(w.player.y, 1);
  CHECK(!w.busy());
}

TEST(a_trigger_runs_once) {
  World w;
  CHECK(play::start(w));
  play::place(w, 4, 9);
  play::press(w, Button::DPAD_RIGHT);
  Index steps = Game::find(w.game.vars, "steps");
  CHECK(w.flag(play::flag(w, "stepped")));
  CHECK_EQ(w.var(steps), 1);
  play::press(w, Button::DPAD_LEFT);
  play::press(w, Button::DPAD_RIGHT);
  CHECK_EQ(w.var(steps), 1);
}

TEST(draw_tile_hook_replaces_tile_drawing) {
  World w;
  CHECK(play::start(w));
  int drawn = 0;
  w.hooks.draw_tile = [&](World &, uint8_t, Point) { drawn++; };
  w.render(0);
  CHECK_EQ(drawn, 16 * 16);
}

TEST(renders_without_a_sheet) {
  World w;
  CHECK(play::start(w));
  w.render(0);
  const uint8_t *px = fake::screen_pixels();
  // background colour from the file on an empty... every pixel is drawn
  CHECK(px[0] != 0 || px[1] != 0 || px[2] != 0);
}

#include "rpg_world.hpp"

using namespace blit;
using namespace chili::rpg;

TEST(a_tap_moves_one_tile_and_slides) {
  World w;
  CHECK(play::start(w));
  play::frame(w, Button::DPAD_DOWN);
  CHECK_EQ(w.player.y, 2);
  CHECK(w.busy());          // still sliding
  CHECK(w.player.facing == Dir::down);
  play::settle(w);
  CHECK(!w.busy());
  CHECK_EQ(w.state.turn, 1);
}

TEST(a_tap_a_bit_longer_than_a_step_still_moves_one_tile) {
  World w;
  CHECK(play::start(w));
  // step_ms is 100 in the fixture; a normal tap can last longer than that
  for(int t = 0; t < 200; t += 10) play::frame(w, Button::DPAD_DOWN);
  play::settle(w);
  CHECK_EQ(w.player.y, 2);
}

TEST(holding_waits_then_keeps_walking_one_step_per_slide) {
  World w;
  CHECK(play::start(w));
  // a step at once, then after repeat_ms (250) one step per step_ms (100):
  // steps at 0, 250, 350, 450 and 550 ms
  for(int t = 0; t < 600; t += 10) play::frame(w, Button::DPAD_DOWN);
  CHECK_EQ(w.player.y, 6);
}

TEST(a_press_during_a_slide_is_not_lost) {
  World w;
  CHECK(play::start(w));
  play::frame(w, Button::DPAD_DOWN);
  play::frame(w);
  play::frame(w, Button::DPAD_RIGHT);  // the first step still slides
  play::settle(w);
  CHECK_EQ(w.player.y, 2);
  CHECK_EQ(w.player.x, 2);
}

TEST(repeat_ms_comes_from_the_rules) {
  World w;
  CHECK(play::start(w, fixture::edit("\"bag\": 4", "\"bag\": 4, \"repeat_ms\": 100")));
  CHECK_EQ(w.game.repeat_ms, 100);
  // steps at 0, 100, 200, 300 and 400 ms
  for(int t = 0; t < 450; t += 10) play::frame(w, Button::DPAD_DOWN);
  CHECK_EQ(w.player.y, 6);
}

TEST(walls_take_no_turn) {
  World w;
  CHECK(play::start(w));
  play::press(w, Button::DPAD_UP);
  CHECK_EQ(w.player.y, 1);
  CHECK_EQ(w.state.turn, 0);
  CHECK(w.player.facing == Dir::up);
}

TEST(b_waits_a_turn) {
  World w;
  CHECK(play::start(w));
  play::press(w, Button::B);
  CHECK_EQ(w.state.turn, 1);
  CHECK_EQ(w.player.x, 1);
}

TEST(no_diagonal_steps_unless_the_file_allows_them) {
  World w;
  CHECK(play::start(w));
  play::press(w, Button::DPAD_DOWN | Button::DPAD_RIGHT);
  CHECK_EQ(w.player.x + w.player.y, 3);  // one tile, not two

  World d;
  CHECK(play::start(d, fixture::edit("\"bag\": 4", "\"bag\": 4, \"diagonal\": true")));
  play::press(d, Button::DPAD_DOWN | Button::DPAD_RIGHT);
  CHECK_EQ(d.player.x, 2);
  CHECK_EQ(d.player.y, 2);
}

TEST(slow_actors_act_every_few_turns) {
  World w;
  CHECK(play::start(w, fixture::edit("\"brain\": \"still\"", "\"brain\": \"wander\", \"speed\": 3")));
  Actor *slime = play::find(w, "slime");
  int moves = 0;
  for(int i = 0; i < 9; i++) {
    int x = slime->x, y = slime->y;
    play::press(w, Button::B, 200);
    if(slime->x != x || slime->y != y) moves++;
  }
  CHECK(moves <= 3);
}

TEST(nothing_moves_while_dialogue_is_open) {
  World w;
  CHECK(play::start(w));
  w.say(0);
  play::press(w, Button::DPAD_DOWN);
  CHECK_EQ(w.player.y, 1);
  CHECK_EQ(w.state.turn, 0);
}

TEST(damage_tiles_hurt) {
  World w;
  CHECK(play::start(w));
  play::place(w, 10, 7);
  int before = play::hp(w, w.player);
  play::press(w, Button::DPAD_RIGHT);
  CHECK_EQ(play::hp(w, w.player), before - 2);
}

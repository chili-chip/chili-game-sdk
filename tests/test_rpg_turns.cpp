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

TEST(holding_keeps_walking_one_step_per_slide) {
  World w;
  CHECK(play::start(w));
  // step_ms is 100 in the fixture: 350 ms held is the first step and three more
  for(int t = 0; t < 350; t += 10) play::frame(w, Button::DPAD_DOWN);
  CHECK_EQ(w.player.y, 5);
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

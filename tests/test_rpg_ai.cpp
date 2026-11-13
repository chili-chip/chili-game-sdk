#include "rpg_world.hpp"

using namespace blit;
using namespace chili::rpg;

// The slime with another brain. It starts at (10, 5).
static std::string with_brain(const std::string &brain) {
  return fixture::edit("\"brain\": \"still\"", "\"brain\": \"" + brain + "\"");
}

TEST(still_stays_put) {
  World w;
  CHECK(play::start(w));
  Actor *slime = play::find(w, "slime");
  play::place(w, 6, 6);
  for(int i = 0; i < 5; i++) play::press(w, Button::B, 150);
  CHECK_EQ(slime->x, 10);
  CHECK_EQ(slime->y, 5);
}

TEST(chase_walks_to_the_player_when_it_sees_it) {
  World w;
  CHECK(play::start(w, with_brain("chase")));
  Actor *slime = play::find(w, "slime");
  play::place(w, 6, 7);
  play::press(w, Button::B, 150);
  CHECK(slime->x == 9 || slime->y == 6);
  for(int i = 0; i < 6; i++) play::press(w, Button::B, 150);
  CHECK_EQ(std::abs(slime->x - 6) + std::abs(slime->y - 7), 1);
  CHECK(play::hp(w, w.player) < 10);  // and then it attacks
}

TEST(chase_does_not_see_through_walls_or_far_away) {
  World w;
  CHECK(play::start(w, with_brain("chase")));
  Actor *slime = play::find(w, "slime");
  play::place(w, 1, 14);  // more than 5 tiles away
  for(int i = 0; i < 3; i++) play::press(w, Button::B, 150);
  CHECK_EQ(slime->x, 10);
  CHECK_EQ(slime->y, 5);
}

TEST(paths_go_round_water) {
  World w;
  CHECK(play::start(w, with_brain("chase")));
  Actor *slime = play::find(w, "slime");
  // water at (5,3) and (6,3); the slime comes from below it
  slime->x = 5;
  slime->y = 4;
  play::place(w, 5, 2);
  for(int i = 0; i < 4; i++) play::press(w, Button::B, 150);
  CHECK_EQ(std::abs(slime->x - 5) + std::abs(slime->y - 2), 1);
}

TEST(guard_goes_back_home) {
  World w;
  CHECK(play::start(w, with_brain("guard")));
  Actor *slime = play::find(w, "slime");
  slime->x = 12;
  slime->y = 12;  // far from home (10, 5), player out of sight
  play::place(w, 1, 1);
  for(int i = 0; i < 12; i++) play::press(w, Button::B, 150);
  CHECK_EQ(slime->x, 10);
  CHECK_EQ(slime->y, 5);
}

TEST(patrol_walks_its_points) {
  // patrol brain on the kind
  World p;
  std::string text = fixture::edit("{ \"kind\": \"slime\", \"x\": 10, \"y\": 5 }",
                                   "{ \"kind\": \"slime\", \"x\": 10, \"y\": 5, \"patrol\": [[12, 5], [10, 5]] }");
  auto at = text.find("\"brain\": \"still\"");
  text.replace(at, 16, "\"brain\": \"patrol\"");
  CHECK(play::start(p, text));
  Actor *slime = play::find(p, "slime");
  play::place(p, 1, 14);
  int seen_far = 0;
  for(int i = 0; i < 10; i++) {
    play::press(p, Button::B, 150);
    if(slime->x == 12) seen_far++;
  }
  CHECK(seen_far > 0);
  CHECK(slime->y == 5);
}

TEST(coward_runs_when_hurt) {
  World w;
  CHECK(play::start(w, with_brain("coward")));
  Actor *slime = play::find(w, "slime");
  slime->stats[w.game.hp] = 1;  // 25% of 4, under the 30% default
  play::place(w, 9, 5);
  play::press(w, Button::B, 150);
  CHECK_EQ(slime->x, 11);
  CHECK_EQ(play::hp(w, w.player), 10);  // it ran instead of attacking
}

TEST(wander_moves_sometimes) {
  World w;
  CHECK(play::start(w, with_brain("wander")));
  Actor *slime = play::find(w, "slime");
  play::place(w, 1, 14);
  int moves = 0;
  for(int i = 0; i < 12; i++) {
    int x = slime->x, y = slime->y;
    play::press(w, Button::B, 150);
    moves += slime->x != x || slime->y != y;
  }
  CHECK(moves > 0);
}

TEST(custom_brains_come_from_code) {
  World w;
  CHECK(play::start(w, with_brain("hop")));
  int calls = 0;
  w.hooks.brains["hop"] = [&](World &world, Actor &a) {
    calls++;
    world.step(a, 0, 1);
  };
  Actor *slime = play::find(w, "slime");
  play::press(w, Button::B, 150);
  CHECK_EQ(calls, 1);
  CHECK_EQ(slime->y, 6);
}

TEST(an_unknown_custom_brain_stands_still) {
  World w;
  CHECK(play::start(w, with_brain("nobody")));
  Actor *slime = play::find(w, "slime");
  play::press(w, Button::B, 150);
  CHECK_EQ(slime->x, 10);
}

#include "rpg_world.hpp"

using namespace blit;
using namespace chili::rpg;

// The slime stands at (10, 5); the player steps into it from (9, 5).
static Actor *next_to_slime(World &w) {
  play::place(w, 9, 5);
  return play::find(w, "slime");
}

TEST(walking_into_an_enemy_attacks_it) {
  World w;
  CHECK(play::start(w));
  Actor *slime = next_to_slime(w);
  play::press(w, Button::DPAD_RIGHT);
  CHECK_EQ(w.player.x, 9);           // did not move
  CHECK_EQ(play::hp(w, *slime), 1);  // hp 4, hit for atk 3 - def 0
}

TEST(enemies_hit_back_next_to_the_player) {
  World w;
  CHECK(play::start(w));
  next_to_slime(w);
  play::press(w, Button::B);  // wait: the still slime attacks
  CHECK_EQ(play::hp(w, w.player), 9);  // atk 2 - def 1
}

TEST(a_kill_gives_xp_drops_and_counts_as_gone) {
  World w;
  CHECK(play::start(w));
  Actor *slime = next_to_slime(w);
  play::press(w, Button::DPAD_RIGHT);
  play::press(w, Button::DPAD_RIGHT);
  CHECK(!slime->alive);
  CHECK_EQ(w.state.xp, 3);
  CHECK_EQ(w.count(play::item(w, "jelly")), 0);
  CHECK_EQ(w.item_at(slime->x, slime->y), play::item(w, "jelly"));  // 100% drop, on the floor
  CHECK(w.state.rooms[0].gone & 2);
  // it stays dead when the player comes back
  w.warp(1, 1, 1, false);
  w.warp(0, 1, 1, false);
  CHECK(play::find(w, "slime") == nullptr);
}

TEST(the_damage_hook_replaces_the_formula) {
  World w;
  CHECK(play::start(w));
  w.hooks.damage = [](World &, const Actor &a, const Actor &) { return a.is_player() ? 100 : 0; };
  Actor *slime = next_to_slime(w);
  play::press(w, Button::DPAD_RIGHT);
  CHECK(!slime->alive);
}

TEST(zero_damage_misses) {
  World w;
  CHECK(play::start(w));
  w.hooks.damage = [](World &, const Actor &, const Actor &) { return 0; };
  next_to_slime(w);
  play::press(w, Button::B);
  CHECK_EQ(play::hp(w, w.player), 10);
}

TEST(spread_and_crit_stay_in_range) {
  World w;
  CHECK(play::start(w, fixture::edit("\"bag\": 4", "\"bag\": 4, \"spread\": 1, \"crit\": 50")));
  Actor *slime = play::find(w, "slime");
  bool saw_crit = false;
  for(int i = 0; i < 50; i++) {
    slime->stats[w.game.hp] = 999;
    slime->alive = true;
    w.attack(w.player, *slime);
    int d = 999 - play::hp(w, *slime);
    CHECK(d >= 2 && d <= 8);  // 3 +- 1, doubled on a crit
    if(d > 4) saw_crit = true;
  }
  CHECK(saw_crit);
}

TEST(player_death_ends_the_game) {
  World w;
  CHECK(play::start(w));
  w.hurt(w.player, 100);
  CHECK(w.result() == Result::lost);
}

TEST(a_lose_event_can_save_the_player) {
  World w;
  CHECK(play::start(w, fixture::edit("\"songs\":", "\"lose\": [ { \"heal\": 5 }, { \"warp\": \"cave\", \"x\": 2, \"y\": 2 } ],\n  \"songs\":")));
  w.hurt(w.player, 100);
  CHECK(w.result() == Result::playing);
  CHECK_EQ(play::hp(w, w.player), 5);
  play::settle(w);
  CHECK(w.room().id == "cave");
}

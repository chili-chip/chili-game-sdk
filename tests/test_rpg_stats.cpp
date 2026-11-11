#include "rpg_world.hpp"

using namespace blit;
using namespace chili::rpg;

TEST(the_player_starts_with_the_file_stats) {
  World w;
  CHECK(play::start(w));
  CHECK_EQ(w.stat(w.player, "hp"), 10);
  CHECK_EQ(w.stat(w.player, "atk"), 3);
  CHECK_EQ(w.stat(w.player, "nothing"), 0);
  CHECK_EQ(w.state.level, 1);
}

TEST(xp_from_the_table_raises_the_level) {
  World w;
  CHECK(play::start(w));
  w.player.stats[w.game.hp] = 4;
  w.gain_xp(4);
  CHECK_EQ(w.state.level, 1);
  w.gain_xp(1);  // 5: level 2
  CHECK_EQ(w.state.level, 2);
  CHECK_EQ(w.stat(w.player, "max_hp"), 12);
  CHECK_EQ(w.stat(w.player, "atk"), 4);
  CHECK_EQ(w.stat(w.player, "hp"), 12);  // full health on a level up
}

TEST(a_big_xp_gain_can_jump_two_levels_and_stops_at_the_top) {
  World w;
  CHECK(play::start(w));
  int ups = 0;
  w.hooks.on_level_up = [&](World &) { ups++; };
  w.gain_xp(500);
  CHECK_EQ(w.state.level, 3);  // the table has two steps
  CHECK_EQ(ups, 2);
}

TEST(equipment_adds_to_stats) {
  World w;
  CHECK(play::start(w));
  w.give(play::item(w, "stick"));
  CHECK_EQ(w.stat(w.player, "atk"), 3);
  w.equip(0);
  CHECK_EQ(w.stat(w.player, "atk"), 5);
  w.equip(0);
  CHECK_EQ(w.stat(w.player, "atk"), 3);
}

TEST(heal_stops_at_max_hp) {
  World w;
  CHECK(play::start(w));
  w.player.stats[w.game.hp] = 5;
  w.heal(3);
  CHECK_EQ(play::hp(w, w.player), 8);
  w.heal(30);
  CHECK_EQ(play::hp(w, w.player), 10);
}

TEST(the_hud_draws) {
  World w;
  CHECK(play::start(w));
  w.render(0);
  const uint8_t *px = fake::screen_pixels();
  // the hp bar is red at its left end
  const uint8_t *p = px + (4 * 128 + 4) * 3;
  CHECK(p[0] > 150 && p[1] < 100);
}

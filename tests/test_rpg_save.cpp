#include "rpg_world.hpp"

using namespace blit;
using namespace chili::rpg;

TEST(a_save_brings_back_where_and_what) {
  World w;
  CHECK(play::start(w));
  w.give(play::item(w, "stick"));
  w.equip(0);
  w.set_flag(play::flag(w, "met"), true);
  w.set_quest(0, 1);
  play::place(w, 9, 5);
  play::press(w, Button::DPAD_RIGHT);  // the stick kills the slime in one hit
  w.save();

  World again;
  CHECK(play::start(again));
  CHECK(again.has_save());
  CHECK(again.load_save());
  CHECK_EQ(again.player.x, 9);
  CHECK_EQ(again.player.y, 5);
  CHECK(again.state.bag[0].equipped);
  CHECK(again.flag(play::flag(again, "met")));
  CHECK_EQ(again.state.quests[0], 1);
  CHECK_EQ(again.state.xp, 3);
  CHECK(play::find(again, "slime") == nullptr);
  CHECK_EQ(again.stat(again.player, "atk"), 5);
}

TEST(an_edited_game_ignores_the_old_save) {
  World w;
  CHECK(play::start(w));
  w.save();
  World other;
  CHECK(play::start(other, fixture::edit("\"title\": \"Test\"", "\"title\": \"Test 2\"")));
  CHECK(!other.has_save());
  CHECK(!other.load_save());
}

TEST(save_tiles_and_the_save_action_save) {
  World w;
  CHECK(play::start(w));
  play::place(w, 6, 13);
  play::press(w, Button::DPAD_RIGHT);
  World again;
  CHECK(play::start(again));
  CHECK(again.load_save());
  CHECK_EQ(again.player.x, 7);
  CHECK_EQ(again.player.y, 13);
}

TEST(the_save_fits_easily) {
  CHECK(sizeof(State) < 2048);
}

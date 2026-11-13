#include "rpg_world.hpp"

using namespace blit;
using namespace chili::rpg;

TEST(items_stack_up_to_their_stack_size) {
  World w;
  CHECK(play::start(w));
  Index jelly = play::item(w, "jelly");
  CHECK_EQ(w.give(jelly, 5), 0);  // stack 3: two slots
  CHECK_EQ(w.count(jelly), 5);
  CHECK_EQ(w.bag_used(), 2);
}

TEST(a_full_bag_returns_what_did_not_fit) {
  World w;
  CHECK(play::start(w));
  Index jelly = play::item(w, "jelly");
  CHECK_EQ(w.give(jelly, 14), 2);  // bag of 4 x stack 3 = 12
  CHECK_EQ(w.count(jelly), 12);
}

TEST(take_removes_and_closes_up_the_bag) {
  World w;
  CHECK(play::start(w));
  Index jelly = play::item(w, "jelly"), key = play::item(w, "key");
  w.give(jelly, 2);
  w.give(key);
  CHECK_EQ(w.take(jelly, 5), 2);
  CHECK_EQ(w.bag_used(), 1);
  CHECK_EQ(w.state.bag[0].item, key);
}

TEST(walking_over_an_item_leaves_it_and_a_takes_it) {
  World w;
  CHECK(play::start(w));
  Index jelly = play::item(w, "jelly");
  play::place(w, 5, 5);
  play::press(w, Button::DPAD_RIGHT);
  CHECK_EQ(w.count(jelly), 0);
  CHECK_EQ(w.item_at(6, 5), jelly);
  play::press(w, Button::A);
  CHECK_EQ(w.count(jelly), 1);
  CHECK_EQ(w.item_at(6, 5), chili::rpg::NONE);
  play::press(w, Button::A);
  CHECK_EQ(w.count(jelly), 1);
}

TEST(a_dropped_item_lies_where_the_player_stands) {
  World w;
  CHECK(play::start(w));
  Index jelly = play::item(w, "jelly");
  w.give(jelly, 2);
  play::place(w, 3, 3);
  w.drop(0);
  CHECK_EQ(w.count(jelly), 0);
  CHECK_EQ(w.item_at(3, 3), jelly);
  w.render(0);
  play::press(w, Button::A);
  CHECK_EQ(w.count(jelly), 2);  // the whole stack comes back
  CHECK_EQ(w.item_at(3, 3), chili::rpg::NONE);
}

TEST(dropped_items_stay_in_their_room_and_in_the_save) {
  World w;
  CHECK(play::start(w));
  Index stick = play::item(w, "stick");
  w.give(stick);
  play::place(w, 3, 3);
  w.drop(0);
  w.warp(1, 1, 1, false);
  CHECK_EQ(w.item_at(3, 3), chili::rpg::NONE);  // not in the cave
  w.warp(0, 1, 1, false);
  CHECK_EQ(w.item_at(3, 3), stick);
  w.save();

  World again;
  CHECK(play::start(again));
  CHECK(again.load_save());
  CHECK_EQ(again.item_at(3, 3), stick);
}

TEST(the_floor_has_room_for_max_floor_drops) {
  World w;
  CHECK(play::start(w));
  Index stick = play::item(w, "stick");
  for(int i = 0; i <= MAX_FLOOR; i++) {
    play::place(w, 1 + i % 10, 1 + i / 10);
    w.give(stick);
    w.drop(0);
  }
  CHECK_EQ(w.count(stick), 1);  // the last one did not fit
}

TEST(using_an_item_runs_its_event_and_uses_it_up) {
  World w;
  CHECK(play::start(w));
  w.give(play::item(w, "jelly"), 2);
  w.player.stats[w.game.hp] = 4;
  w.use_item(0);
  CHECK_EQ(play::hp(w, w.player), 7);
  CHECK_EQ(w.count(play::item(w, "jelly")), 1);
}

TEST(on_use_item_can_take_over) {
  World w;
  CHECK(play::start(w));
  w.give(play::item(w, "jelly"));
  w.hooks.on_use_item = [](World &, Index) { return true; };
  w.player.stats[w.game.hp] = 4;
  w.use_item(0);
  CHECK_EQ(play::hp(w, w.player), 4);
  CHECK_EQ(w.count(play::item(w, "jelly")), 1);
}

TEST(one_item_per_equip_slot) {
  World w;
  CHECK(play::start(w, fixture::edit("{ \"id\": \"key\"", "{ \"id\": \"club\", \"kind\": \"equip\", \"slot\": \"weapon\", \"bonus\": { \"atk\": 5 } },\n    { \"id\": \"key\"")));
  w.give(play::item(w, "stick"));
  w.give(play::item(w, "club"));
  w.equip(0);
  w.equip(1);
  CHECK(!w.state.bag[0].equipped);
  CHECK(w.state.bag[1].equipped);
  CHECK_EQ(w.stat(w.player, "atk"), 8);
}

TEST(key_items_cannot_be_dropped) {
  World w;
  CHECK(play::start(w));
  w.give(play::item(w, "key"));
  w.give(play::item(w, "jelly"));
  w.drop(0);
  CHECK_EQ(w.count(play::item(w, "key")), 1);
  w.drop(1);
  CHECK_EQ(w.count(play::item(w, "jelly")), 0);
}

TEST(the_inventory_screen_uses_equips_and_drops) {
  World w;
  CHECK(play::start(w));
  w.give(play::item(w, "stick"));
  w.give(play::item(w, "jelly"));
  play::press(w, Button::X);
  CHECK(w.screen() == World::Screen::inventory);
  play::press(w, Button::A);       // equip the stick
  CHECK(w.state.bag[0].equipped);
  play::press(w, Button::DPAD_DOWN);
  play::press(w, Button::X);       // arm the drop
  CHECK_EQ(w.bag_used(), 2);
  play::press(w, Button::X);       // drop the jelly
  CHECK_EQ(w.bag_used(), 1);
  CHECK_EQ(w.item_at(w.player.x, w.player.y), play::item(w, "jelly"));
  w.render(0);
  play::press(w, Button::B);
  CHECK(w.screen() == World::Screen::none);
  CHECK_EQ(w.state.turn, 0);       // screens take no turn
}

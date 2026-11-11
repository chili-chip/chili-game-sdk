#include "rpg_world.hpp"

using namespace blit;
using namespace chili::rpg;

// Runs the actions in `json` (a list) on the fixture.
static void run(World &w, const std::string &json) {
  Game &g = w.game;
  // add the actions as an item's use event, then run it
  std::string text = fixture::edit("\"use\": [ { \"heal\": 3 } ]", "\"use\": " + json);
  CHECK(play::start(w, text));
  w.run(g.items[0].use);
}

TEST(flags_and_variables) {
  World w;
  run(w, R"([ { "set": "a" }, { "set": "b" }, { "clear": "a" }, { "add": "gold", "value": 5 }, { "add": "gold", "value": -2 } ])");
  CHECK(!w.flag(play::flag(w, "a")));
  CHECK(w.flag(play::flag(w, "b")));
  CHECK_EQ(w.var(Game::find(w.game.vars, "gold")), 3);
}

TEST(give_take_heal_hurt_stat_xp) {
  World w;
  run(w, R"([ { "give": "jelly", "count": 2 }, { "take": "jelly" }, { "hurt": 4 }, { "heal": 1 }, { "stat": "atk", "value": 2 }, { "xp": 5 } ])");
  CHECK_EQ(w.count(play::item(w, "jelly")), 1);
  // hurt 4, heal 1, then the level up from 5 xp fills hp
  CHECK_EQ(w.state.level, 2);
  CHECK_EQ(w.stat(w.player, "atk"), 6);
  CHECK_EQ(play::hp(w, w.player), 12);
}

TEST(conditions_pick_do_or_else) {
  World w;
  CHECK(play::start(w));
  Actor *door = play::find(w, "door");
  play::place(w, 7, 11);
  play::press(w, Button::DPAD_DOWN);  // locked
  CHECK(w.flag(play::flag(w, "locked")));
  CHECK(door->alive);
  w.give(play::item(w, "key"));
  play::press(w, Button::DPAD_DOWN);  // opens: the key is used and the door removed
  CHECK(!door->alive);
  CHECK_EQ(w.count(play::item(w, "key")), 0);
  CHECK(w.state.rooms[0].gone & 4);
}

TEST(condition_operators) {
  World w;
  run(w, R"([ { "add": "n", "value": 3 } ])");
  Index n = Game::find(w.game.vars, "n");
  auto cond = [&](Condition::Op op, int value) {
    w.game.conditions.push_back({Condition::Type::var, op, n, int16_t(value)});
    Event e;
    e.if_at = w.game.conditions.size() - 1;
    e.if_count = 1;
    return w.check(e);
  };
  CHECK(cond(Condition::Op::eq, 3));
  CHECK(cond(Condition::Op::ne, 2));
  CHECK(cond(Condition::Op::lt, 4));
  CHECK(cond(Condition::Op::le, 3));
  CHECK(cond(Condition::Op::gt, 2));
  CHECK(cond(Condition::Op::ge, 3));
  CHECK(!cond(Condition::Op::gt, 3));
}

TEST(call_runs_code_by_name) {
  World w;
  int calls = 0;
  w.hooks.actions["open_shop"] = [&](World &) { calls++; };
  run(w, R"([ { "call": "open_shop" }, { "call": "missing" } ])");
  CHECK_EQ(calls, 1);
}

TEST(end_win_and_warp) {
  World w;
  run(w, R"([ { "warp": "cave", "x": 3, "y": 4 } ])");
  play::settle(w);
  CHECK(w.room().id == "cave");
  CHECK_EQ(w.player.x, 3);
  World e;
  run(e, R"([ { "end": "win" } ])");
  CHECK(e.result() == Result::won);
}

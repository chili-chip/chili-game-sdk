#include "check.hpp"
#include "rpg_fixture.hpp"

#include "chili/rpg/game.hpp"

#include <string>

using namespace chili::rpg;

static bool has(const std::string &text, const std::string &part) {
  return text.find(part) != std::string::npos;
}

TEST(loads_the_fixture) {
  Game g;
  std::string err = load(fixture::game, g);
  if(!err.empty()) std::printf("%s\n", err.c_str());
  CHECK(err.empty());
  CHECK(g.title == "Test");
  CHECK_EQ(g.background[0], 0x22);
  CHECK_EQ(g.stats.size(), 4);
  CHECK_EQ(g.hp, 0);
  CHECK_EQ(g.max_hp, 1);
  CHECK_EQ(g.atk, 2);
  CHECK_EQ(g.rooms.size(), 2);
  CHECK_EQ(g.rooms[1].w, 32);
  CHECK_EQ(g.start_room, 0);
  CHECK_EQ(g.step_ms, 100);
  CHECK_EQ(g.bag_size, 4);
  CHECK_EQ(g.xp.size(), 2);
  CHECK_EQ(g.gain[g.atk], 1);
}

TEST(turns_ids_into_indexes) {
  Game g;
  CHECK(load(fixture::game, g).empty());
  const Room &home = g.rooms[0];
  CHECK_EQ(home.actors[1].kind, Game::find(g.kinds, "slime"));
  CHECK_EQ(home.warps[0].room, 1);
  CHECK_EQ(home.items[0].item, Game::find(g.items, "jelly"));
  CHECK_EQ(g.kinds[0].talk, Game::find(g.dialogue, "hello"));
  CHECK_EQ(g.items[1].slot, 0);
  CHECK_EQ(g.items[1].bonus[g.atk], 2);
  CHECK(g.items[1].kind == ItemKind::equip);
  // tile keys: '#' is solid, '~' has two frames
  int wall = home.ground_at(0, 0);
  CHECK(g.tiles[wall].solid);
  int water = home.ground_at(5, 3);
  CHECK(g.tiles[water].water);
  CHECK_EQ(g.tiles[water].frame_count, 2);
}

TEST(kind_stats_default_max_hp_to_hp) {
  Game g;
  CHECK(load(fixture::game, g).empty());
  const Kind &slime = g.kinds[1];
  CHECK_EQ(slime.stats[g.hp], 4);
  CHECK_EQ(slime.stats[g.max_hp], 4);
  CHECK_EQ(slime.stats[g.def], 0);
}

TEST(reads_events_into_conditions_and_actions) {
  Game g;
  CHECK(load(fixture::game, g).empty());
  const Event &bump = g.kinds[2].bump;
  CHECK_EQ(bump.if_count, 1);
  CHECK_EQ(bump.do_count, 2);
  CHECK_EQ(bump.else_count, 1);
  const Condition &c = g.conditions[bump.if_at];
  CHECK(c.type == Condition::Type::has);
  CHECK_EQ(c.target, Game::find(g.items, "key"));
  CHECK_EQ(c.value, 1);
  CHECK(g.actions[bump.do_at + 1].type == Action::Type::remove);
  // flags and variables are named by use
  CHECK(Game::find(g.flags, "locked") != NONE);
  CHECK(Game::find(g.vars, "steps") != NONE);
}

TEST(reads_dialogue_choices_and_gotos) {
  Game g;
  CHECK(load(fixture::game, g).empty());
  const Dialogue &d = g.dialogue[0];
  CHECK_EQ(d.lines.size(), 4);
  CHECK(d.lines[0].who == "Elder");    // from the kind
  CHECK_EQ(d.lines[0].face, 16);
  CHECK_EQ(d.lines[1].event.if_count, 1);
  CHECK_EQ(d.lines[2].choices.size(), 2);
  CHECK_EQ(d.lines[2].choices[0].next, 3);
  CHECK_EQ(d.lines[2].choices[1].next, NONE);
}

TEST(reads_quests) {
  Game g;
  CHECK(load(fixture::game, g).empty());
  const Quest &q = g.quests[0];
  CHECK_EQ(q.stages.size(), 2);
  CHECK(q.stages[0].goal.type == Goal::Type::collect);
  CHECK_EQ(q.stages[0].goal.count, 2);
  CHECK(q.stages[1].goal.type == Goal::Type::defeat);
  CHECK_EQ(q.stages[1].goal.target, 1);
}

TEST(hash_changes_with_the_file) {
  Game a, b;
  CHECK(load(fixture::game, a).empty());
  CHECK(load(fixture::edit("\"title\": \"Test\"", "\"title\": \"Other\""), b).empty());
  CHECK(a.hash != 0);
  CHECK(a.hash != b.hash);
}

TEST(errors_name_the_field) {
  Game g;
  CHECK(has(load(fixture::edit("\"bonus\": { \"atk\": 2 }", "\"bonus\": { \"atk\": \"x\" }"), g),
            "items[1].bonus.atk: not a whole number"));
  CHECK(has(load(fixture::edit("{ \"kind\": \"slime\"", "{ \"kind\": \"blob\""), g),
            "rooms[0].actors[1].kind: no kind \"blob\""));
  CHECK(has(load(fixture::edit("\"to\": \"cave\"", "\"to\": \"nowhere\""), g),
            "rooms[0].warps[0].to: no room \"nowhere\""));
  CHECK(has(load(fixture::edit("\"goto\": \"thanks\"", "\"goto\": \"bye\""), g),
            "dialogue[0].lines[2].choices[0].goto: no line \"bye\""));
  CHECK(has(load(fixture::edit("{ \"heal\": 3 }", "{ \"fly\": 3 }"), g),
            "items[0].use[0]: unknown action"));
  CHECK(has(load(fixture::edit("\"format\": \"chili-rpg\"", "\"format\": \"other\""), g), "format"));
  CHECK(has(load(fixture::edit("\"#..............>\"", "\"#.......>\""), g), "rooms[0].ground[8]: rows must all be"));
  CHECK(has(load(fixture::edit("\"#....~~........#\"", "\"#....??........#\""), g), "rooms[0].ground[3]: no tile '?'"));
  CHECK(has(load(fixture::edit("\"x\": 7, \"y\": 1", "\"x\": 70, \"y\": 1"), g), "rooms[0].actors[0].x"));
  CHECK(has(load(fixture::edit("{ \"id\": \"hp\",", "{ \"id\": \"health\","), g), "stats: needs \"hp\""));
  CHECK(has(load("[1, 2", g), "line 1"));
}

TEST(rejects_duplicate_ids) {
  Game g;
  CHECK(has(load(fixture::edit("{ \"id\": \"stick\"", "{ \"id\": \"jelly\""), g), "items[1].id: \"jelly\" is used twice"));
}

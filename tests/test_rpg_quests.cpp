#include "rpg_world.hpp"

using namespace blit;
using namespace chili::rpg;

// Talks to the elder (at 7,1) from (6,1) and reads to the choice.
static void talk_to_elder(World &w) {
  play::place(w, 6, 1);
  play::press(w, Button::DPAD_RIGHT, 1500);
}

static void next(World &w) { play::press(w, Button::A, 1500); }

TEST(bumping_a_friendly_actor_talks) {
  World w;
  CHECK(play::start(w));
  talk_to_elder(w);
  CHECK(w.talking());
  CHECK_EQ(w.player.x, 6);
  CHECK_EQ(w.state.turn, 0);
}

TEST(a_also_talks_to_the_actor_in_front) {
  World w;
  CHECK(play::start(w));
  play::place(w, 6, 1);
  w.player.facing = Dir::right;
  play::press(w, Button::A, 500);
  CHECK(w.talking());
}

TEST(lines_with_failing_conditions_are_skipped) {
  World w;
  CHECK(play::start(w));
  talk_to_elder(w);  // "Hello there."
  next(w);           // skips "Back again?" (not met yet): "Bring me jelly?"
  CHECK(w.talking());
  // pick "No": the dialogue ends, but the line's own action ran
  play::press(w, Button::DPAD_DOWN, 100);
  next(w);
  CHECK(!w.talking());
  CHECK(w.flag(play::flag(w, "met")));
  CHECK_EQ(w.state.quests[0], 0);
}

TEST(a_choice_runs_its_actions_and_jumps) {
  World w;
  CHECK(play::start(w));
  talk_to_elder(w);
  next(w);
  next(w);  // "Yes": quest stage 1, then "Thank you!"
  CHECK(w.talking());
  CHECK_EQ(w.state.quests[0], 1);
  next(w);
  CHECK(!w.talking());
}

TEST(collect_then_defeat_then_done) {
  World w;
  CHECK(play::start(w));
  w.set_quest(0, 1);
  w.give(play::item(w, "jelly"), 2);
  w.check_quests();
  CHECK_EQ(w.state.quests[0], 2);                // stage 2: beat a slime
  CHECK_EQ(w.count(play::item(w, "stick")), 1);  // stage 1's reward
  play::place(w, 9, 5);
  play::press(w, Button::DPAD_RIGHT);
  play::press(w, Button::DPAD_RIGHT);
  CHECK(w.state.quests[0] == State::DONE);
}

TEST(defeats_only_count_while_the_stage_is_active) {
  World w;
  CHECK(play::start(w));
  play::place(w, 9, 5);
  play::press(w, Button::DPAD_RIGHT);
  play::press(w, Button::DPAD_RIGHT);  // slime dead before the quest
  w.set_quest(0, 2);
  w.check_quests();
  CHECK_EQ(w.state.quests[0], 2);
}

TEST(talk_reach_and_flag_goals) {
  std::string text = fixture::edit(
    "{ \"log\": \"Bring 2 jellies.\", \"goal\": { \"collect\": \"jelly\", \"count\": 2 }, \"done\": [ { \"give\": \"stick\" } ] }",
    "{ \"goal\": { \"talk\": \"elder\" } }, { \"goal\": { \"reach\": \"home\", \"x\": 3, \"y\": 3 } }, { \"goal\": { \"flag\": \"go\" } }");
  World w;
  CHECK(play::start(w, text));
  w.set_quest(0, 1);
  talk_to_elder(w);
  next(w);
  play::press(w, Button::DPAD_DOWN, 100);  // "No", so the quest is not restarted
  next(w);
  CHECK(!w.talking());
  CHECK_EQ(w.state.quests[0], 2);
  play::place(w, 3, 2);
  play::press(w, Button::DPAD_DOWN);
  CHECK_EQ(w.state.quests[0], 3);
  w.set_flag(play::flag(w, "go"), true);
  w.check_quests();
  CHECK_EQ(w.state.quests[0], 4);  // stage 4: beat a slime
}

TEST(the_quest_log_opens_and_closes) {
  World w;
  CHECK(play::start(w));
  w.set_quest(0, 1);
  play::press(w, Button::Y);
  CHECK(w.screen() == World::Screen::quests);
  w.render(0);
  play::press(w, Button::B);
  CHECK(w.screen() == World::Screen::none);
}

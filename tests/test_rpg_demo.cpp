// Plays the demo RPG's story from start to end, with the real game file.

#include "rpg_world.hpp"

#include <fstream>
#include <sstream>

using namespace blit;
using namespace chili::rpg;

static std::string demo_file() {
  std::ifstream in(CHILI_DEMO_RPG);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}

static void read_all(World &w) {
  for(int i = 0; i < 12 && w.talking(); i++) play::press(w, Button::A, 2500);
}

// Walks next to the first living actor of a kind and bumps it.
static void bump(World &w, const char *kind, int from_dx = -1) {
  Actor *a = play::find(w, kind);
  CHECK(a != nullptr);
  if(!a) return;
  play::place(w, a->x + from_dx, a->y);
  play::press(w, from_dx < 0 ? Button::DPAD_RIGHT : Button::DPAD_LEFT, 2500);
}

TEST(the_demo_loads) {
  World w;
  CHECK(play::start(w, demo_file()));
  CHECK(w.game.title == "Pepper Village");
  CHECK_EQ(w.game.kinds.size(), 8);
}

TEST(the_demo_story_plays_to_the_end) {
  World w;
  CHECK(play::start(w, demo_file()));
  Index stone = Game::find(w.game.quests, "stone");

  bump(w, "elder", 1);  // "Will you bring it back?"
  play::press(w, Button::A, 2500);
  CHECK(w.talking());
  play::press(w, Button::A, 2500);  // Yes
  read_all(w);
  CHECK_EQ(w.state.quests[stone], 1);

  bump(w, "smith", 1);
  read_all(w);
  CHECK_EQ(w.count(play::item(w, "sword")), 1);
  CHECK_EQ(w.state.quests[stone], 2);
  w.equip(0);

  w.warp(Game::find(w.game.rooms, "cave"), 2, 2, false);
  w.player.stats[w.game.hp] = 99;  // the test is about the story, not the fights
  for(int i = 0; i < 3; i++) {
    Actor *s = play::find(w, "spider");
    while(s && s->alive) {
      play::place(w, s->x - 1, s->y);
      if(w.blocked(w.player.x, w.player.y) || w.actor_at(w.player.x, w.player.y) != &w.player) play::place(w, s->x + 1, s->y);
      w.attack(w.player, *s);
      play::settle(w, 200);
    }
  }
  play::press(w, Button::B);  // a turn passes: quest goals are checked
  CHECK_EQ(w.state.quests[stone], 3);

  // the skeleton has the key to the stone room
  Actor *sk = play::find(w, "skeleton");
  while(sk->alive) w.attack(w.player, *sk);
  play::settle(w);
  CHECK_EQ(w.count(play::item(w, "key")), 1);
  bump(w, "door");
  CHECK(play::find(w, "door") == nullptr);
  play::place(w, 26, 5);
  play::press(w, Button::DPAD_RIGHT);
  CHECK_EQ(w.state.quests[stone], 4);

  w.warp(Game::find(w.game.rooms, "village"), 10, 10, false);
  bump(w, "elder", 1);
  read_all(w);
  CHECK(w.state.quests[stone] == State::DONE);
  CHECK_EQ(w.count(play::item(w, "ring")), 1);
  CHECK(w.result() == Result::won);
}

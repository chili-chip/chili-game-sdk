#pragma once

// Helpers for the World tests: load the fixture and play frames.

#include "check.hpp"
#include "fake_api.hpp"
#include "rpg_fixture.hpp"

#include "chili/rpg/world.hpp"

#include <cstdio>
#include <string>

namespace play {

using namespace chili::rpg;

// A world on the fixture (or a variant of it), with the screen set up.
inline bool start(World &w, const std::string &text = fixture::game) {
  blit::set_screen_mode(blit::ScreenMode::hires);
  std::string err = w.load(text);
  if(!err.empty()) std::printf("load: %s\n", err.c_str());
  return err.empty();
}

// One 10 ms frame with `held` buttons down.
inline void frame(World &w, uint32_t held = 0) {
  fake::hold(held);
  w.update(10);
}

// Lets slides, fades and text finish without pressing anything.
inline void settle(World &w, int ms = 600) {
  for(int t = 0; t < ms; t += 10) frame(w);
}

// A press for one frame, then time for it to play out.
inline void press(World &w, uint32_t button, int ms = 300) {
  frame(w, button);
  settle(w, ms);
}

inline Index kind(World &w, const char *id) { return Game::find(w.game.kinds, id); }
inline Index item(World &w, const char *id) { return Game::find(w.game.items, id); }
inline Index flag(World &w, const char *id) { return Game::find(w.game.flags, id); }

inline Actor *find(World &w, const char *kind_id) {
  for(auto &a : w.actors)
    if(a.alive && a.kind == kind(w, kind_id)) return &a;
  return nullptr;
}

inline int hp(World &w, const Actor &a) { return a.stats[w.game.hp]; }

// Puts the player on a tile, no turn taken.
inline void place(World &w, int x, int y) {
  w.player.x = x;
  w.player.y = y;
}

}

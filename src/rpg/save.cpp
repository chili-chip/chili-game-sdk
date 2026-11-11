// Saving the RPG state with chili::Save. The game file's hash is the magic
// number, so a save never loads into a different (or edited) game file.

#include "chili/rpg/world.hpp"

#include "chili/save.hpp"

namespace chili::rpg {

namespace {
constexpr uint16_t SAVE_VERSION = 1;
}

void World::save() const {
  Save<State> s(game.hash, SAVE_VERSION, save_slot);
  s.data = state;
  s.data.x = player.x;
  s.data.y = player.y;
  s.data.facing = player.facing;
  s.data.stats = player.stats;
  s.store();
}

bool World::has_save() const {
  Save<State> s(game.hash, SAVE_VERSION, save_slot);
  return s.load();
}

bool World::load_save() {
  Save<State> s(game.hash, SAVE_VERSION, save_slot);
  if(!s.load()) return false;
  new_game();
  state = s.data;
  if(state.room < 0 || state.room >= Index(game.rooms.size())) {
    new_game();
    return false;
  }
  player.stats = state.stats;
  enter_room(state.room, state.x, state.y);
  player.facing = state.facing;
  return true;
}

}

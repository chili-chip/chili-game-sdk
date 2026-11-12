#pragma once

#include "32blit.hpp"
#include "chili/audio.hpp"
#include "chili/dialogue.hpp"
#include "chili/fx.hpp"
#include "chili/rpg/game.hpp"
#include "chili/scene.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// A turn-based RPG on a grid, played from a game file.
//
//   chili::rpg::World world;
//   world.sheet = Surface::load(asset_sheet);
//   world.load(std::string_view((const char *)asset_game, asset_game_length));
//   ... update(): world.update(dt);   render(): world.render(time);
//
// The player moves one tile, then every other actor takes its turn. Walking
// into an enemy attacks it; walking into a friendly actor talks to it.
namespace chili::rpg {

// Someone on the map: the player, a person, an enemy, a door.
struct Actor {
  Index kind = NONE;  // NONE for the player
  int8_t spawn = -1;  // index in Room::actors; -1 for the player
  uint8_t x = 0, y = 0;
  uint8_t home_x = 0, home_y = 0;
  Dir facing = Dir::down;
  Stats stats{};      // base stats; stats[hp] is the current hp
  bool alive = true;
  uint8_t patrol_at = 0;

  // Drawing only: the sprite is drawn `ox, oy` px away from its tile,
  // shrinking to 0 over the step time, so steps slide smoothly.
  int8_t ox = 0, oy = 0;
  uint16_t slide_ms = 0;
  uint16_t flash_ms = 0;
  uint8_t walk = 0;   // walk frame

  bool is_player() const { return kind == NONE; }
};

// One bag slot. Equipped items stay in the bag.
struct BagSlot {
  Index item = NONE;
  uint8_t count = 0;
  bool equipped = false;
};

// What changed in each room: actors gone (dead, or removed), items picked
// up and one-time triggers fired. One bit per entry in the room's lists.
struct RoomState {
  uint32_t gone = 0;
  uint32_t taken = 0;
  uint32_t fired = 0;
};

// Everything a save holds. Plain data, so it goes into chili::Save as is.
struct State {
  Index room = 0;
  uint8_t x = 0, y = 0;
  Dir facing = Dir::down;
  uint8_t level = 1;
  Stats stats{};
  int32_t xp = 0;
  uint32_t turn = 0;
  BagSlot bag[MAX_BAG];
  uint32_t flags[MAX_FLAGS / 32] = {};
  int16_t vars[MAX_VARS] = {};
  uint8_t quests[MAX_QUESTS] = {};    // 0 not started, 1.. stage, DONE
  int16_t progress[MAX_QUESTS] = {};  // talks or defeats towards the stage goal
  RoomState rooms[MAX_ROOMS];

  static constexpr uint8_t DONE = 0xff;
};

enum class Result : uint8_t { playing, won, lost };

class World;

// Code that changes the rules. Every hook is optional: a game from the
// no-code maker runs with none of them set.
struct Hooks {
  // Damage of one hit. Default: max(1, atk - def), then the file's spread
  // and crit.
  std::function<int(World &, const Actor &attacker, const Actor &defender)> damage;
  // After the player's level went up (stats already raised).
  std::function<void(World &)> on_level_up;
  // Before an item is used. Return true when you handled it: the item's own
  // event then does not run and nothing is used up.
  std::function<bool(World &, Index item)> on_use_item;
  // Actions the file runs with {"call": "name"}.
  std::map<std::string, std::function<void(World &)>> actions;
  // Brains the file names in a kind's "brain" (any name that is not built in).
  std::map<std::string, std::function<void(World &, Actor &)>> brains;
  // Draws one tile instead of the sprite sheet.
  std::function<void(World &, uint8_t tile, blit::Point at)> draw_tile;
  // Plays a sound instead of the built-in effects.
  std::function<void(Sound)> sound;
};

class World : public Scene {
public:
  World();

  // Loads a game file and starts a new game. Returns "" or the error.
  std::string load(std::string_view text);
  // Starts over: stats, bag, flags, quests and rooms back to the start.
  void new_game();

  void update(uint32_t dt) override;
  void render(uint32_t time) override;

  Result result() const { return result_; }
  // True while the world waits for something other than a move: a step
  // still sliding, dialogue, a screen or a warp.
  bool busy() const;

  // -------------------------------------------------------------------------
  // What is loaded and what is happening

  Game game;
  State state;
  Actor player;
  std::vector<Actor> actors;  // in the current room
  Hooks hooks;
  // The sprite sheet: 8x8 sprites, numbered left to right, top to bottom.
  // Without one, the world draws coloured squares.
  blit::Surface *sheet = nullptr;
  bool show_hud = true;
  int save_slot = 0;

  const Room &room() const { return game.rooms[state.room]; }
  bool flag(Index f) const;
  void set_flag(Index f, bool on);
  int var(Index v) const { return v >= 0 && v < MAX_VARS ? state.vars[v] : 0; }
  // A stat, with equipment bonuses for the player.
  int stat(const Actor &a, Index s) const;
  int stat(const Actor &a, std::string_view id) const { return stat(a, Game::find(game.stats, id)); }
  // How many of an item the player has (equipped ones count).
  int count(Index item) const;
  int bag_used() const;
  // The actor standing on a tile, or nullptr.
  Actor *actor_at(int x, int y);
  // True for solid or water tiles and the room's edges. With `sight`, only
  // solid tiles block.
  bool blocked(int x, int y, bool sight = false) const;
  const TileType &tile(int x, int y) const;

  // -------------------------------------------------------------------------
  // Doing things. Events call these; so can hooks and custom brains.

  bool check(const Event &e) const;
  // Runs the event's "do" actions when its conditions pass, else its "else".
  // `self` is the actor the event belongs to (for "remove").
  void run(const Event &e, Actor *self = nullptr);
  void run_action(const Action &a, Actor *self = nullptr);

  // Moves an actor one tile when it is free. Returns true when it moved.
  bool step(Actor &a, int dx, int dy);
  void attack(Actor &attacker, Actor &defender);
  void hurt(Actor &a, int amount);
  void heal(int amount);
  void gain_xp(int amount);
  // Puts items in the bag. Returns how many did not fit.
  int give(Index item, int count = 1);
  // Takes items out of the bag. Returns how many it took.
  int take(Index item, int count = 1);
  void use_item(int bag_index);
  // Equips an item, or takes it off when it is already equipped.
  void equip(int bag_index);
  void drop(int bag_index);
  void say(Index dialogue);
  void set_quest(Index quest, int stage);
  void finish_quest(Index quest);
  // Checks every active quest's goal and moves finished ones on.
  void check_quests();
  void warp(Index room, int x, int y, bool fade = true);
  void play(Sound s);
  void end(bool won);

  // Saves the state to the console's save slot. A save only loads into the
  // same game file (the file's hash is the save's magic number).
  void save() const;
  bool load_save();
  bool has_save() const;

  // Screens on top of the map.
  enum class Screen : uint8_t { none, inventory, quests };
  void open(Screen s);
  Screen screen() const { return screen_; }
  TextBox &text_box() { return box; }
  bool talking() const { return box.active(); }

  // Floating text over a tile.
  void popup(int x, int y, std::string_view text, blit::Pen color);
  // The top left of the view, in map pixels.
  blit::Point camera() const;

  fx::Particles particles{48};
  fx::FloatingText floats{8};
  fx::Shake shake;

  // Drawing, for games that draw their own HUD or screens.
  void draw_sprite(int sprite, blit::Point at, bool flip = false) const;
  void draw_hud();
  void draw_inventory(uint32_t time);
  void draw_quests(uint32_t time);

private:
  void enter_room(Index room, int x, int y);
  void player_turn(int dx, int dy);
  void interact(Actor &a);
  void after_step(Actor &a);
  void others_turn();
  void end_turn();
  void think(Actor &a);
  bool next_to(const Actor &a, const Actor &b) const;
  bool sees(const Actor &a, const Actor &b) const;
  bool path_step(const Actor &a, int tx, int ty, int &dx, int &dy) const;
  void step_away(Actor &a, const Actor &from);
  void wander(Actor &a);
  void die(Actor &a);
  void level_up();
  void next_line();
  void update_screen(uint32_t pressed);
  void remove_slot(int i);
  Event bump_event(const Actor &a) const;
  Index talk_of(const Actor &a) const;
  int damage(const Actor &attacker, const Actor &defender);
  void pad(uint32_t held, int &dx, int &dy) const;
  void slide(Actor &a, int dx, int dy, int px);
  void play_room_music();

  TextBox box;
  Index talk = NONE;  // dialogue being shown
  int line = -1;
  uint32_t talk_gen = 0;

  Screen screen_ = Screen::none;
  int cursor = 0;
  bool drop_armed = false;

  Result result_ = Result::playing;
  std::vector<audio::Song> songs;
  Index song = NONE;

  // a warp waiting for the fade
  Index warp_room = NONE;
  uint8_t warp_x = 0, warp_y = 0;
  uint16_t fade_ms = 0;
  uint16_t hold_ms = 0;         // how long the D-pad has been held
  int8_t queued_dx = 0, queued_dy = 0;  // a D-pad press made while busy
  bool fading_in = false;
};

}

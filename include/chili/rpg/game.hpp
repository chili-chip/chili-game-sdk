#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// The content of an RPG, loaded from a game file (.rpg, JSON). Everything a
// player sees or a designer tunes lives here. Ids from the file ("slime",
// "jelly") become indexes into these lists when the file loads, so nothing
// compares strings while the game runs.
//
// The file format is described in format/schema.json and on
// https://game-sdk.chilichip.eu/rpg/game-file/
namespace chili::rpg {

// Limits, checked when a file loads.
constexpr int MAX_STATS = 8;
constexpr int MAX_TILES = 90;       // one printable character each
constexpr int MAX_ROOMS = 32;
constexpr int MIN_ROOM = 16;        // tiles per side
constexpr int MAX_ROOM = 64;
constexpr int MAX_MAP_CELLS = 24 * 1024;  // all rooms together, per layer
constexpr int MAX_KINDS = 64;
constexpr int MAX_ACTORS = 32;      // per room, also items and triggers
constexpr int MAX_ITEMS = 128;
constexpr int MAX_BAG = 32;
constexpr int MAX_SLOTS = 8;        // equipment slots
constexpr int MAX_FLAGS = 256;
constexpr int MAX_VARS = 64;
constexpr int MAX_QUESTS = 32;
constexpr int MAX_STAGES = 8;
constexpr int MAX_CHOICES = 4;
constexpr int MAX_TEXT = 32 * 1024; // dialogue and item text together
constexpr int MAX_ACTIONS = 1024;   // conditions and actions, each

using Index = int16_t;
constexpr Index NONE = -1;

using Stats = std::array<int16_t, MAX_STATS>;

enum class Dir : uint8_t { down, up, left, right };

// Built-in sounds that actions and the world play. World::hooks.sound can
// replace them.
enum class Sound : uint8_t { step, bump, hit, hurt, miss, pickup, door, level, save, quest, win, lose, count };

// ---------------------------------------------------------------------------
// Events: a list of conditions, then the actions to run when they all pass
// (and, optionally, the ones to run when they do not).

struct Condition {
  enum class Type : uint8_t { flag, not_flag, var, has, quest, quest_done, stat };
  enum class Op : uint8_t { eq, ne, lt, le, gt, ge };
  Type type;
  Op op = Op::eq;
  Index target = NONE;  // flag, variable, item, quest or stat
  int16_t value = 0;
};

struct Action {
  enum class Type : uint8_t {
    set, clear, add, give, take, heal, hurt, stat, xp, warp, say, quest, quest_done,
    sound, music, save, end, remove, call
  };
  Type type;
  Index target = NONE;  // flag, variable, item, stat, room, dialogue, quest, sound, song or call
  int16_t value = 0;    // amount, stage, or 1 for "win" with end
  uint8_t x = 0, y = 0; // warp
};

// Ranges into Game::conditions and Game::actions.
struct Event {
  uint16_t if_at = 0, if_count = 0;
  uint16_t do_at = 0, do_count = 0;
  uint16_t else_at = 0, else_count = 0;
  bool empty() const { return !do_count && !else_count; }
};

// ---------------------------------------------------------------------------
// Content

struct StatDef {
  std::string id;
  std::string label;  // short, for the HUD: "ATK"
  int16_t start = 0;
};

struct TileType {
  char key = '.';
  uint8_t frames[4] = {0, 0, 0, 0};  // sprites, for animation
  uint8_t frame_count = 1;
  uint16_t frame_ms = 400;
  bool solid = false;   // blocks walking and sight
  bool water = false;   // blocks walking, not sight
  bool save = false;    // stepping on it saves the game
  int16_t damage = 0;   // hurts whoever steps on it
};

enum class Brain : uint8_t { still, wander, chase, guard, patrol, coward, custom };

struct Drop {
  Index item = NONE;
  uint8_t chance = 100;  // percent
  uint8_t count = 1;
};

// A kind of actor: an enemy, a person, a door...
struct Kind {
  std::string id;
  std::string name;
  uint8_t sprites[4] = {0, 0, 0, 0};  // by Dir
  bool flip_left = true;  // no "left" sprite: draw "right" mirrored
  uint8_t frames = 1;     // walk frames, the sprites after each one
  Brain brain = Brain::still;
  Index custom = NONE;    // index in Game::brains for Brain::custom
  uint8_t sight = 5;      // tiles
  uint8_t home = 4;       // tiles a guard strays from home
  uint8_t flee = 30;      // percent of max hp a coward runs at
  uint8_t speed = 1;      // acts every `speed` turns
  bool friendly = false;  // bumping talks instead of attacking
  Stats stats{};
  int16_t xp = 0;
  std::vector<Drop> drops;
  Index talk = NONE;      // dialogue
  Event bump;             // runs when the player bumps a friendly actor
  Event death;
};

struct Spot {
  uint8_t x, y;
};

struct RoomActor {
  Index kind = NONE;
  uint8_t x = 0, y = 0;
  Index talk = NONE;      // overrides the kind's
  Event bump;             // overrides the kind's when set
  std::vector<Spot> patrol;
};

struct RoomItem {
  Index item = NONE;
  uint8_t x = 0, y = 0;
  uint8_t count = 1;
};

struct Warp {
  uint8_t x = 0, y = 0;
  Index room = NONE;
  uint8_t tx = 0, ty = 0;
};

struct Trigger {
  uint8_t x = 0, y = 0;
  bool once = true;
  Event event;
};

struct Room {
  std::string id;
  std::string name;
  uint8_t w = 16, h = 16;
  std::vector<uint8_t> ground;  // tile type per cell
  std::vector<uint8_t> top;     // tile type, or EMPTY
  std::vector<RoomActor> actors;
  std::vector<RoomItem> items;
  std::vector<Warp> warps;
  std::vector<Trigger> triggers;
  Index music = NONE;

  static constexpr uint8_t EMPTY = 0xff;
  uint8_t ground_at(int x, int y) const { return ground[y * w + x]; }
  uint8_t top_at(int x, int y) const { return top.empty() ? EMPTY : top[y * w + x]; }
};

enum class ItemKind : uint8_t { use, equip, key, quest };

struct Item {
  std::string id;
  std::string name;
  std::string text;  // description
  uint8_t icon = 0;
  ItemKind kind = ItemKind::use;
  uint8_t stack = 1;
  uint16_t price = 0;
  Index slot = NONE;  // equip slot
  Stats bonus{};      // added to stats while equipped
  Event use;
};

struct Choice {
  std::string text;
  Event event;
  Index next = NONE;  // line in the same dialogue, or NONE to end
};

struct Line {
  std::string label;  // for "goto"
  std::string who;    // name shown in the box
  int16_t face = -1;  // portrait sprite, or -1
  std::string text;
  Event event;        // conditions to show the line, actions after it
  std::vector<Choice> choices;
};

struct Dialogue {
  std::string id;
  std::vector<Line> lines;
};

struct Goal {
  enum class Type : uint8_t { none, talk, collect, defeat, reach, flag };
  Type type = Type::none;
  Index target = NONE;  // kind, item, room or flag
  int16_t count = 1;
  uint8_t x = 0, y = 0; // reach
};

struct Stage {
  std::string log;
  Goal goal;
  Event done;
};

struct Quest {
  std::string id;
  std::string title;
  std::vector<Stage> stages;
};

struct Song {
  std::string id;
  uint16_t step_ms = 150;
  std::string lead, bass, drums;
};

struct Game {
  std::string title;
  std::string sprites;  // sprite sheet file name, for the player app
  uint32_t hash = 0;    // of the file text; saves only load into the same file
  uint8_t background[3] = {0, 0, 0};

  Index start_room = 0;
  uint8_t start_x = 0, start_y = 0;
  uint8_t player_sprites[4] = {0, 0, 0, 0};
  bool player_flip = true;
  uint8_t player_frames = 1;

  uint16_t step_ms = 80;
  uint16_t repeat_ms = 250;  // a held D-pad waits this long before it keeps walking
  bool diagonal = false;
  uint8_t bag_size = 16;

  std::vector<StatDef> stats;
  Index hp = NONE, max_hp = NONE, atk = NONE, def = NONE;
  std::vector<int32_t> xp;  // total xp needed for level 2, 3, ...
  Stats gain{};             // added to the base stats on each level up
  uint8_t spread = 0;       // damage varies by up to this much either way
  uint8_t crit = 0;         // percent chance of double damage

  std::vector<TileType> tiles;
  std::vector<Room> rooms;
  std::vector<Kind> kinds;
  std::vector<Item> items;
  std::vector<std::string> slots;
  std::vector<Dialogue> dialogue;
  std::vector<Quest> quests;
  std::vector<Song> songs;

  std::vector<std::string> flags, vars, calls, brains;  // names, by index
  std::vector<Condition> conditions;
  std::vector<Action> actions;

  Event lose;  // runs when the player dies; empty: the game ends (lost)

  template<typename T>
  static Index find(const std::vector<T> &list, std::string_view id) {
    for(size_t i = 0; i < list.size(); i++)
      if(list[i].id == id) return Index(i);
    return NONE;
  }
  static Index find(const std::vector<std::string> &names, std::string_view id) {
    for(size_t i = 0; i < names.size(); i++)
      if(names[i] == id) return Index(i);
    return NONE;
  }
};

// Reads a game file into `game`. Returns "" or an error with where it is,
// such as "items[3].bonus.atk: not a number". The text can be freed after.
std::string load(std::string_view text, Game &game);

// The name of a sound, as the file writes it ("hit"), and back.
const char *sound_name(Sound s);

}

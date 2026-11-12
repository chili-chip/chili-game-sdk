// Reads a game file (JSON) into a Game. Two passes: the first collects the
// ids of everything that can be named, the second reads the rest and turns
// names into indexes.

#include "chili/rpg/game.hpp"

#include "rpg/json.hpp"

#include <cstdio>

namespace chili::rpg {

namespace {

using json::Ref;

const char *const sound_names[] = {"step", "bump", "hit", "hurt", "miss", "pickup", "door", "level", "save", "quest", "win", "lose"};
static_assert(sizeof(sound_names) / sizeof(sound_names[0]) == int(Sound::count));

uint32_t fnv1a(std::string_view text) {
  uint32_t h = 2166136261u;
  for(char c : text) h = (h ^ uint8_t(c)) * 16777619u;
  return h ? h : 1;
}

std::string at(const std::string &path, const char *key) {
  return path.empty() ? key : path + "." + key;
}

std::string at(const std::string &path, int i) {
  return path + "[" + std::to_string(i) + "]";
}

struct Loader {
  Game &g;
  std::string error;

  bool fail(const std::string &path, const std::string &what) {
    if(error.empty()) error = path.empty() ? what : path + ": " + what;
    return false;
  }

  // -------------------------------------------------------------------------
  // Plain fields

  bool integer(Ref obj, const char *key, const std::string &path, int lo, int hi, int &out) {
    Ref v = obj.get(key);
    if(!v) return true;
    if(!v.is_int()) return fail(at(path, key), "not a whole number");
    if(v.integer() < lo || v.integer() > hi)
      return fail(at(path, key), "must be " + std::to_string(lo) + " to " + std::to_string(hi));
    out = v.integer();
    return true;
  }

  template<typename T>
  bool number(Ref obj, const char *key, const std::string &path, int lo, int hi, T &out) {
    int n = out;
    if(!integer(obj, key, path, lo, hi, n)) return false;
    out = T(n);
    return true;
  }

  bool required_number(Ref obj, const char *key, const std::string &path, int lo, int hi, uint8_t &out) {
    if(!obj.get(key)) return fail(at(path, key), "missing");
    return number(obj, key, path, lo, hi, out);
  }

  bool text(Ref obj, const char *key, const std::string &path, std::string &out, bool required = false) {
    Ref v = obj.get(key);
    if(!v) return required ? fail(at(path, key), "missing") : true;
    if(!v.is_string()) return fail(at(path, key), "not a string");
    out = v.string();
    return true;
  }

  bool flag(Ref obj, const char *key, const std::string &path, bool &out) {
    Ref v = obj.get(key);
    if(!v) return true;
    if(!v.is_bool()) return fail(at(path, key), "not true or false");
    out = v.boolean();
    return true;
  }

  bool array(Ref obj, const char *key, const std::string &path, Ref &out, int limit) {
    out = obj.get(key);
    if(!out) return true;
    if(!out.is_array()) return fail(at(path, key), "not a list");
    if(out.size() > limit) return fail(at(path, key), "more than " + std::to_string(limit));
    return true;
  }

  // A name from `list` (ids of the first pass). Missing is fine unless
  // `required`; then `out` stays NONE.
  template<typename List>
  bool ref(Ref obj, const char *key, const std::string &path, const List &list, const char *what, Index &out,
           bool required = false) {
    Ref v = obj.get(key);
    if(!v || v.is_null()) return required ? fail(at(path, key), "missing") : true;
    if(!v.is_string()) return fail(at(path, key), "not a string");
    out = Game::find(list, v.string());
    if(out == NONE) return fail(at(path, key), std::string("no ") + what + " \"" + v.string() + "\"");
    return true;
  }

  // A flag, variable, call or brain name: added the first time it is used.
  bool name(Ref v, const std::string &path, std::vector<std::string> &names, int limit, Index &out) {
    if(!v.is_string() || v.string().empty()) return fail(path, "not a name");
    out = Game::find(names, v.string());
    if(out != NONE) return true;
    if(int(names.size()) >= limit) return fail(path, "more than " + std::to_string(limit) + " names");
    names.push_back(v.string());
    out = Index(names.size() - 1);
    return true;
  }

  // Sprite indexes: a number, or a list of numbers (animation frames).
  bool sprites(Ref v, const std::string &path, uint8_t *out, uint8_t &count, int max) {
    if(v.is_int()) {
      if(v.integer() < 0 || v.integer() > 255) return fail(path, "must be 0 to 255");
      out[0] = v.integer();
      count = 1;
      return true;
    }
    if(!v.is_array() || v.size() < 1 || v.size() > max)
      return fail(path, "a sprite number, or a list of 1 to " + std::to_string(max));
    count = 0;
    for(auto s : v.items()) {
      if(!s.is_int() || s.integer() < 0 || s.integer() > 255) return fail(at(path, count), "must be 0 to 255");
      out[count++] = s.integer();
    }
    return true;
  }

  // {"hp": 4, "atk": 1}: values for named stats.
  bool stat_values(Ref obj, const char *key, const std::string &path, Stats &out) {
    Ref v = obj.get(key);
    if(!v) return true;
    if(!v.is_object()) return fail(at(path, key), "not an object");
    for(auto m : v.members()) {
      std::string p = at(at(path, key), std::string(m.key()).c_str());
      Index s = Game::find(g.stats, m.key());
      if(s == NONE) return fail(p, "no stat \"" + std::string(m.key()) + "\"");
      if(!m.is_int()) return fail(p, "not a whole number");
      if(m.integer() < -9999 || m.integer() > 9999) return fail(p, "must be -9999 to 9999");
      out[s] = m.integer();
    }
    return true;
  }

  // A direction sprite set: "sprite" (all directions) and "sprites" by name.
  bool facing(Ref obj, const std::string &path, uint8_t *out, bool &flip, uint8_t &frames) {
    Ref s = obj.get("sprite");
    if(s) {
      if(!s.is_int() || s.integer() < 0 || s.integer() > 255) return fail(at(path, "sprite"), "must be 0 to 255");
      for(int i = 0; i < 4; i++) out[i] = s.integer();
    }
    if(Ref by = obj.get("sprites")) {
      if(!by.is_object()) return fail(at(path, "sprites"), "not an object");
      const char *names[4] = {"down", "up", "left", "right"};
      for(int i = 0; i < 4; i++) {
        Ref d = by.get(names[i]);
        if(!d) continue;
        if(!d.is_int() || d.integer() < 0 || d.integer() > 255)
          return fail(at(at(path, "sprites"), names[i]), "must be 0 to 255");
        out[i] = d.integer();
        if(i == int(Dir::left)) flip = false;
      }
    }
    if(!flag(obj, "flip", path, flip)) return false;
    return number(obj, "frames", path, 1, 4, frames);
  }

  // -------------------------------------------------------------------------
  // Events

  bool condition(Ref c, const std::string &path) {
    if(!c.is_object()) return fail(path, "not an object");
    if(int(g.conditions.size()) >= MAX_ACTIONS) return fail(path, "too many conditions in the game");
    Condition out{Condition::Type::flag};
    int value = 1;

    if(Ref v = c.get("flag")) {
      if(!name(v, at(path, "flag"), g.flags, MAX_FLAGS, out.target)) return false;
    } else if(Ref v = c.get("not")) {
      out.type = Condition::Type::not_flag;
      if(!name(v, at(path, "not"), g.flags, MAX_FLAGS, out.target)) return false;
    } else if(Ref v = c.get("var")) {
      out.type = Condition::Type::var;
      value = 0;
      if(!name(v, at(path, "var"), g.vars, MAX_VARS, out.target)) return false;
    } else if(c.get("has")) {
      out.type = Condition::Type::has;
      out.op = Condition::Op::ge;
      if(!ref(c, "has", path, g.items, "item", out.target)) return false;
      if(!integer(c, "count", path, 1, 999, value)) return false;
    } else if(c.get("quest")) {
      if(!ref(c, "quest", path, g.quests, "quest", out.target)) return false;
      bool done = false;
      if(c.get("done")) {
        out.type = Condition::Type::quest_done;
        if(!flag(c, "done", path, done)) return false;
        out.op = done ? Condition::Op::eq : Condition::Op::ne;
      } else {
        out.type = Condition::Type::quest;
        value = 0;
        if(!integer(c, "stage", path, 0, MAX_STAGES, value)) return false;
      }
    } else if(c.get("stat")) {
      out.type = Condition::Type::stat;
      value = 0;
      if(!ref(c, "stat", path, g.stats, "stat", out.target)) return false;
    } else {
      return fail(path, "unknown condition");
    }

    if(Ref op = c.get("op")) {
      const char *ops[] = {"=", "!=", "<", "<=", ">", ">="};
      std::string o = op.string();
      int found = -1;
      for(int i = 0; i < 6; i++)
        if(o == ops[i]) found = i;
      if(found < 0) return fail(at(path, "op"), "one of = != < <= > >=");
      out.op = Condition::Op(found);
    }
    if(!integer(c, "value", path, -32768, 32767, value)) return false;
    out.value = value;
    g.conditions.push_back(out);
    return true;
  }

  bool action(Ref a, const std::string &path) {
    if(!a.is_object()) return fail(path, "not an object");
    if(int(g.actions.size()) >= MAX_ACTIONS) return fail(path, "too many actions in the game");
    Action out{Action::Type::set};
    int value = 0;
    using T = Action::Type;

    if(Ref v = a.get("set")) {
      if(!name(v, at(path, "set"), g.flags, MAX_FLAGS, out.target)) return false;
    } else if(Ref v = a.get("clear")) {
      out.type = T::clear;
      if(!name(v, at(path, "clear"), g.flags, MAX_FLAGS, out.target)) return false;
    } else if(Ref v = a.get("add")) {
      out.type = T::add;
      if(!name(v, at(path, "add"), g.vars, MAX_VARS, out.target)) return false;
      if(!integer(a, "value", path, -32768, 32767, value)) return false;
    } else if(a.get("give") || a.get("take")) {
      out.type = a.get("give") ? T::give : T::take;
      if(!ref(a, out.type == T::give ? "give" : "take", path, g.items, "item", out.target)) return false;
      value = 1;
      if(!integer(a, "count", path, 1, 999, value)) return false;
    } else if(a.get("heal") || a.get("hurt") || a.get("xp")) {
      const char *key = a.get("heal") ? "heal" : a.get("hurt") ? "hurt" : "xp";
      out.type = a.get("heal") ? T::heal : a.get("hurt") ? T::hurt : T::xp;
      if(!a.get(key).is_int()) return fail(at(path, key), "not a whole number");
      if(!integer(a, key, path, 0, 9999, value)) return false;
    } else if(a.get("stat")) {
      out.type = T::stat;
      if(!ref(a, "stat", path, g.stats, "stat", out.target)) return false;
      if(!integer(a, "value", path, -9999, 9999, value)) return false;
    } else if(a.get("warp")) {
      out.type = T::warp;
      if(!ref(a, "warp", path, g.rooms, "room", out.target)) return false;
      if(!required_number(a, "x", path, 0, MAX_ROOM - 1, out.x)) return false;
      if(!required_number(a, "y", path, 0, MAX_ROOM - 1, out.y)) return false;
    } else if(a.get("say")) {
      out.type = T::say;
      if(!ref(a, "say", path, g.dialogue, "dialogue", out.target)) return false;
    } else if(a.get("quest")) {
      if(!ref(a, "quest", path, g.quests, "quest", out.target)) return false;
      if(a.get("done")) {
        out.type = T::quest_done;
      } else {
        out.type = T::quest;
        if(!a.get("stage")) return fail(at(path, "stage"), "missing");
        if(!integer(a, "stage", path, 0, MAX_STAGES, value)) return false;
      }
    } else if(Ref v = a.get("sound")) {
      out.type = T::sound;
      std::string s = v.string();
      for(int i = 0; i < int(Sound::count); i++)
        if(s == sound_names[i]) out.target = i;
      if(out.target == NONE) return fail(at(path, "sound"), "no sound \"" + s + "\"");
    } else if(a.get("music")) {
      out.type = T::music;
      if(!ref(a, "music", path, g.songs, "song", out.target)) return false;
    } else if(a.get("save")) {
      out.type = T::save;
    } else if(Ref v = a.get("end")) {
      out.type = T::end;
      std::string s = v.string();
      if(s != "win" && s != "lose") return fail(at(path, "end"), "\"win\" or \"lose\"");
      value = s == "win";
    } else if(a.get("remove")) {
      out.type = T::remove;
    } else if(Ref v = a.get("call")) {
      out.type = T::call;
      if(!name(v, at(path, "call"), g.calls, 64, out.target)) return false;
    } else {
      return fail(path, "unknown action");
    }
    out.value = value;
    g.actions.push_back(out);
    return true;
  }

  bool actions(Ref list, const std::string &path, uint16_t &first, uint16_t &count) {
    if(!list.is_array()) return fail(path, "not a list of actions");
    first = g.actions.size();
    int i = 0;
    for(auto a : list.items())
      if(!action(a, at(path, i++))) return false;
    count = g.actions.size() - first;
    return true;
  }

  // The "if", "do" and "else" keys of an object.
  bool event_keys(Ref obj, const std::string &path, Event &e) {
    if(Ref cs = obj.get("if")) {
      if(!cs.is_array()) return fail(at(path, "if"), "not a list of conditions");
      e.if_at = g.conditions.size();
      int i = 0;
      for(auto c : cs.items())
        if(!condition(c, at(at(path, "if"), i++))) return false;
      e.if_count = g.conditions.size() - e.if_at;
    }
    if(Ref d = obj.get("do"))
      if(!actions(d, at(path, "do"), e.do_at, e.do_count)) return false;
    if(Ref d = obj.get("else"))
      if(!actions(d, at(path, "else"), e.else_at, e.else_count)) return false;
    return true;
  }

  // An event field: a list of actions, or an object with if/do/else.
  bool event(Ref obj, const char *key, const std::string &path, Event &e) {
    Ref v = obj.get(key);
    if(!v) return true;
    if(v.is_array()) return actions(v, at(path, key), e.do_at, e.do_count);
    if(v.is_object()) return event_keys(v, at(path, key), e);
    return fail(at(path, key), "a list of actions, or an object with if, do and else");
  }

  // -------------------------------------------------------------------------
  // Pass 1: ids

  template<typename T>
  bool ids(Ref root, const char *key, std::vector<T> &out, int limit, bool required = false) {
    Ref list;
    if(!array(root, key, "", list, limit)) return false;
    if(!list) return required ? fail(key, "missing") : true;
    int i = 0;
    for(auto v : list.items()) {
      std::string p = at(key, i++);
      if(!v.is_object()) return fail(p, "not an object");
      T t;
      if(!text(v, "id", p, t.id, true)) return false;
      if(t.id.empty()) return fail(at(p, "id"), "empty");
      if(Game::find(out, t.id) != NONE) return fail(at(p, "id"), "\"" + t.id + "\" is used twice");
      out.push_back(std::move(t));
    }
    return true;
  }

  bool collect_ids(Ref root) {
    if(!ids(root, "stats", g.stats, MAX_STATS, true)) return false;
    if(!ids(root, "rooms", g.rooms, MAX_ROOMS, true)) return false;
    if(!ids(root, "kinds", g.kinds, MAX_KINDS)) return false;
    if(!ids(root, "items", g.items, MAX_ITEMS)) return false;
    if(!ids(root, "dialogue", g.dialogue, 255)) return false;
    if(!ids(root, "quests", g.quests, MAX_QUESTS)) return false;
    if(!ids(root, "songs", g.songs, 32)) return false;

    Ref slots;
    if(!array(root, "slots", "", slots, MAX_SLOTS)) return false;
    int i = 0;
    for(auto s : slots.items()) {
      if(!s.is_string()) return fail(at("slots", i), "not a string");
      g.slots.push_back(s.string());
      i++;
    }
    return true;
  }

  // -------------------------------------------------------------------------
  // Pass 2

  bool header(Ref root) {
    if(root.get("format").string() != "chili-rpg") return fail("format", "must be \"chili-rpg\"");
    if(root.get("version").integer() != 1) return fail("version", "must be 1");
    if(!text(root, "title", "", g.title)) return false;
    if(!text(root, "sprites", "", g.sprites)) return false;

    if(Ref bg = root.get("background")) {
      std::string s = bg.string();
      unsigned r, gr, b;
      if(s.size() != 7 || s[0] != '#' || sscanf(s.c_str() + 1, "%2x%2x%2x", &r, &gr, &b) != 3)
        return fail("background", "a colour like \"#222323\"");
      g.background[0] = r;
      g.background[1] = gr;
      g.background[2] = b;
    }

    Ref rules = root.get("rules");
    if(rules && !rules.is_object()) return fail("rules", "not an object");
    if(!number(rules, "step_ms", "rules", 0, 1000, g.step_ms)) return false;
    if(!number(rules, "repeat_ms", "rules", 0, 1000, g.repeat_ms)) return false;
    if(!flag(rules, "diagonal", "rules", g.diagonal)) return false;
    if(!number(rules, "bag", "rules", 1, MAX_BAG, g.bag_size)) return false;
    if(!number(rules, "spread", "rules", 0, 99, g.spread)) return false;
    if(!number(rules, "crit", "rules", 0, 100, g.crit)) return false;

    Ref player = root.get("player");
    if(!player.is_object()) return fail("player", "missing");
    return facing(player, "player", g.player_sprites, g.player_flip, g.player_frames);
  }

  bool stats(Ref root) {
    int i = 0;
    for(auto s : root.get("stats").items()) {
      std::string p = at("stats", i);
      StatDef &d = g.stats[i++];
      if(!text(s, "label", p, d.label)) return false;
      if(d.label.empty()) d.label = d.id;
      if(!number(s, "start", p, -9999, 9999, d.start)) return false;
    }
    g.hp = Game::find(g.stats, "hp");
    g.max_hp = Game::find(g.stats, "max_hp");
    g.atk = Game::find(g.stats, "atk");
    g.def = Game::find(g.stats, "def");
    if(g.hp == NONE || g.max_hp == NONE) return fail("stats", "needs \"hp\" and \"max_hp\"");

    Ref levels = root.get("levels");
    if(!levels) return true;
    Ref xp;
    if(!array(levels, "xp", "levels", xp, 99)) return false;
    int last = 0;
    i = 0;
    for(auto v : xp.items()) {
      std::string p = at("levels.xp", i++);
      if(!v.is_int() || v.integer() <= last) return fail(p, "must be a number above the one before");
      last = v.integer();
      g.xp.push_back(last);
    }
    return stat_values(levels, "gain", "levels", g.gain);
  }

  bool tiles(Ref root) {
    Ref list;
    if(!array(root, "tiles", "", list, MAX_TILES)) return false;
    if(!list || list.size() == 0) return fail("tiles", "missing");
    int i = 0;
    for(auto t : list.items()) {
      std::string p = at("tiles", i++);
      TileType tile;
      std::string key;
      if(!text(t, "key", p, key, true)) return false;
      if(key.size() != 1 || key[0] <= ' ' || key[0] > '~') return fail(at(p, "key"), "one character");
      tile.key = key[0];
      for(auto &other : g.tiles)
        if(other.key == tile.key) return fail(at(p, "key"), "'" + key + "' is used twice");
      Ref s = t.get("sprite");
      if(!s) return fail(at(p, "sprite"), "missing");
      if(!sprites(s, at(p, "sprite"), tile.frames, tile.frame_count, 4)) return false;
      if(!number(t, "frame_ms", p, 50, 5000, tile.frame_ms)) return false;
      if(!flag(t, "solid", p, tile.solid)) return false;
      if(!flag(t, "water", p, tile.water)) return false;
      if(!flag(t, "save", p, tile.save)) return false;
      if(!number(t, "damage", p, 0, 999, tile.damage)) return false;
      g.tiles.push_back(tile);
    }
    return true;
  }

  int tile_of(char c) const {
    for(size_t i = 0; i < g.tiles.size(); i++)
      if(g.tiles[i].key == c) return int(i);
    return -1;
  }

  bool layer(Ref room, const char *key, const std::string &path, Room &r, std::vector<uint8_t> &out, bool top) {
    Ref rows = room.get(key);
    if(!rows) return top ? true : fail(at(path, key), "missing");
    if(!rows.is_array()) return fail(at(path, key), "not a list of rows");
    if(top && rows.size() != r.h) return fail(at(path, key), "needs as many rows as \"ground\"");
    if(!top) {
      if(rows.size() < MIN_ROOM || rows.size() > MAX_ROOM)
        return fail(at(path, key), "16 to 64 rows");
      r.h = rows.size();
    }
    int y = 0;
    for(auto row : rows.items()) {
      std::string p = at(at(path, key), y);
      if(!row.is_string()) return fail(p, "not a string");
      std::string s = row.string();
      if(y == 0 && !top) {
        if(s.size() < MIN_ROOM || s.size() > MAX_ROOM) return fail(p, "16 to 64 tiles wide");
        r.w = s.size();
      }
      if(int(s.size()) != r.w) return fail(p, "rows must all be " + std::to_string(r.w) + " wide");
      for(char c : s) {
        if(top && c == ' ') {
          out.push_back(Room::EMPTY);
          continue;
        }
        int t = tile_of(c);
        if(t < 0) return fail(p, std::string("no tile '") + c + "'");
        out.push_back(t);
      }
      y++;
    }
    return true;
  }

  bool spot(Ref obj, const std::string &path, const Room &r, const char *kx, const char *ky, uint8_t &x, uint8_t &y) {
    if(!required_number(obj, kx, path, 0, r.w - 1, x)) return false;
    return required_number(obj, ky, path, 0, r.h - 1, y);
  }

  bool rooms(Ref root) {
    int cells = 0;
    int i = 0;
    for(auto room : root.get("rooms").items()) {
      std::string p = at("rooms", i);
      Room &r = g.rooms[i++];
      if(!text(room, "name", p, r.name)) return false;
      if(!layer(room, "ground", p, r, r.ground, false)) return false;
      if(!layer(room, "top", p, r, r.top, true)) return false;
      cells += r.w * r.h;
      if(cells > MAX_MAP_CELLS) return fail(at(p, "ground"), "all rooms together are too big");
      if(!ref(room, "music", p, g.songs, "song", r.music)) return false;

      Ref list;
      if(!array(room, "actors", p, list, MAX_ACTORS)) return false;
      int n = 0;
      for(auto a : list.items()) {
        std::string ap = at(at(p, "actors"), n++);
        RoomActor ra;
        if(!ref(a, "kind", ap, g.kinds, "kind", ra.kind, true)) return false;
        if(!spot(a, ap, r, "x", "y", ra.x, ra.y)) return false;
        if(!ref(a, "talk", ap, g.dialogue, "dialogue", ra.talk)) return false;
        if(!event(a, "bump", ap, ra.bump)) return false;
        Ref patrol;
        if(!array(a, "patrol", ap, patrol, 16)) return false;
        int k = 0;
        for(auto pt : patrol.items()) {
          std::string pp = at(at(ap, "patrol"), k++);
          if(!pt.is_array() || pt.size() != 2 || !pt.at(0).is_int() || !pt.at(1).is_int() || pt.at(0).integer() < 0 ||
             pt.at(0).integer() >= r.w || pt.at(1).integer() < 0 || pt.at(1).integer() >= r.h)
            return fail(pp, "an [x, y] in the room");
          ra.patrol.push_back({uint8_t(pt.at(0).integer()), uint8_t(pt.at(1).integer())});
        }
        r.actors.push_back(std::move(ra));
      }

      if(!array(room, "items", p, list, MAX_ACTORS)) return false;
      n = 0;
      for(auto it : list.items()) {
        std::string ip = at(at(p, "items"), n++);
        RoomItem ri;
        if(!ref(it, "item", ip, g.items, "item", ri.item, true)) return false;
        if(!spot(it, ip, r, "x", "y", ri.x, ri.y)) return false;
        if(!number(it, "count", ip, 1, 99, ri.count)) return false;
        r.items.push_back(ri);
      }

      if(!array(room, "warps", p, list, MAX_ACTORS)) return false;
      n = 0;
      for(auto w : list.items()) {
        std::string wp = at(at(p, "warps"), n++);
        Warp warp;
        if(!spot(w, wp, r, "x", "y", warp.x, warp.y)) return false;
        if(!ref(w, "to", wp, g.rooms, "room", warp.room, true)) return false;
        if(!required_number(w, "tx", wp, 0, MAX_ROOM - 1, warp.tx)) return false;
        if(!required_number(w, "ty", wp, 0, MAX_ROOM - 1, warp.ty)) return false;
        r.warps.push_back(warp);
      }

      if(!array(room, "triggers", p, list, MAX_ACTORS)) return false;
      n = 0;
      for(auto t : list.items()) {
        std::string tp = at(at(p, "triggers"), n++);
        Trigger trig;
        if(!spot(t, tp, r, "x", "y", trig.x, trig.y)) return false;
        if(!flag(t, "once", tp, trig.once)) return false;
        if(!event_keys(t, tp, trig.event)) return false;
        r.triggers.push_back(trig);
      }
    }
    // warp targets are checked once every room has its size
    for(size_t ri = 0; ri < g.rooms.size(); ri++)
      for(size_t w = 0; w < g.rooms[ri].warps.size(); w++) {
        const Warp &warp = g.rooms[ri].warps[w];
        const Room &to = g.rooms[warp.room];
        if(warp.tx >= to.w || warp.ty >= to.h)
          return fail(at(at(at("rooms", int(ri)), "warps"), int(w)), "tx, ty is outside \"" + to.id + "\"");
      }
    return true;
  }

  bool start(Ref root) {
    Ref s = root.get("start");
    if(!s.is_object()) return fail("start", "missing");
    if(!ref(s, "room", "start", g.rooms, "room", g.start_room, true)) return false;
    return spot(s, "start", g.rooms[g.start_room], "x", "y", g.start_x, g.start_y);
  }

  bool kinds(Ref root) {
    int i = 0;
    for(auto k : root.get("kinds").items()) {
      std::string p = at("kinds", i);
      Kind &kind = g.kinds[i++];
      if(!text(k, "name", p, kind.name)) return false;
      if(kind.name.empty()) kind.name = kind.id;
      if(!facing(k, p, kind.sprites, kind.flip_left, kind.frames)) return false;

      if(Ref b = k.get("brain")) {
        const char *brains[] = {"still", "wander", "chase", "guard", "patrol", "coward"};
        std::string s = b.string();
        int found = -1;
        for(int n = 0; n < 6; n++)
          if(s == brains[n]) found = n;
        if(found >= 0) {
          kind.brain = Brain(found);
        } else {
          // any other name is a brain the game registers in C++
          kind.brain = Brain::custom;
          if(!name(b, at(p, "brain"), g.brains, 32, kind.custom)) return false;
        }
      }
      if(!number(k, "sight", p, 0, 32, kind.sight)) return false;
      if(!number(k, "home", p, 0, 32, kind.home)) return false;
      if(!number(k, "flee", p, 0, 100, kind.flee)) return false;
      if(!number(k, "speed", p, 1, 3, kind.speed)) return false;
      if(!flag(k, "friendly", p, kind.friendly)) return false;
      if(!stat_values(k, "stats", p, kind.stats)) return false;
      if(!k.get("stats") || !k.get("stats").get("max_hp")) kind.stats[g.max_hp] = kind.stats[g.hp];
      if(kind.stats[g.max_hp] <= 0) kind.stats[g.max_hp] = kind.stats[g.hp] = 1;
      if(kind.stats[g.hp] <= 0) kind.stats[g.hp] = kind.stats[g.max_hp];
      if(!number(k, "xp", p, 0, 9999, kind.xp)) return false;

      Ref drops;
      if(!array(k, "drops", p, drops, 8)) return false;
      int n = 0;
      for(auto d : drops.items()) {
        std::string dp = at(at(p, "drops"), n++);
        Drop drop;
        if(!ref(d, "item", dp, g.items, "item", drop.item, true)) return false;
        if(!number(d, "chance", dp, 0, 100, drop.chance)) return false;
        if(!number(d, "count", dp, 1, 99, drop.count)) return false;
        kind.drops.push_back(drop);
      }
      if(!ref(k, "talk", p, g.dialogue, "dialogue", kind.talk)) return false;
      if(!event(k, "bump", p, kind.bump)) return false;
      if(!event(k, "death", p, kind.death)) return false;
    }
    return true;
  }

  int text_size = 0;

  bool count_text(const std::string &s, const std::string &path) {
    text_size += s.size();
    if(text_size > MAX_TEXT) return fail(path, "more than 32 KB of text in the game");
    return true;
  }

  bool items(Ref root) {
    int i = 0;
    for(auto it : root.get("items").items()) {
      std::string p = at("items", i);
      Item &item = g.items[i++];
      if(!text(it, "name", p, item.name)) return false;
      if(item.name.empty()) item.name = item.id;
      if(!text(it, "text", p, item.text)) return false;
      if(!count_text(item.text, at(p, "text"))) return false;
      if(!number(it, "icon", p, 0, 255, item.icon)) return false;
      if(Ref k = it.get("kind")) {
        const char *kinds[] = {"use", "equip", "key", "quest"};
        std::string s = k.string();
        int found = -1;
        for(int n = 0; n < 4; n++)
          if(s == kinds[n]) found = n;
        if(found < 0) return fail(at(p, "kind"), "one of use, equip, key, quest");
        item.kind = ItemKind(found);
      }
      if(!number(it, "stack", p, 1, 99, item.stack)) return false;
      if(!number(it, "price", p, 0, 65535, item.price)) return false;
      if(!ref(it, "slot", p, g.slots, "slot", item.slot, item.kind == ItemKind::equip)) return false;
      if(item.kind == ItemKind::equip) item.stack = 1;
      if(!stat_values(it, "bonus", p, item.bonus)) return false;
      if(!event(it, "use", p, item.use)) return false;
    }
    return true;
  }

  bool dialogue(Ref root) {
    int i = 0;
    for(auto d : root.get("dialogue").items()) {
      std::string p = at("dialogue", i);
      Dialogue &dlg = g.dialogue[i++];
      std::string who;
      int face = -1;
      if(!text(d, "who", p, who)) return false;
      if(!integer(d, "face", p, -1, 255, face)) return false;
      Ref lines;
      if(!array(d, "lines", p, lines, 64)) return false;
      if(!lines || lines.size() == 0) return fail(at(p, "lines"), "missing");

      // labels first, so a choice can jump forward
      for(auto l : lines.items()) {
        Line line;
        text(l, "id", "", line.label);
        dlg.lines.push_back(std::move(line));
      }
      int n = 0;
      for(auto l : lines.items()) {
        std::string lp = at(at(p, "lines"), n);
        Line &line = dlg.lines[n++];
        line.who = who;
        line.face = face;
        if(!text(l, "who", lp, line.who)) return false;
        if(!number(l, "face", lp, -1, 255, line.face)) return false;
        // "who" can name a kind: its name, and its sprite as the face
        Index kind = Game::find(g.kinds, line.who);
        if(kind != NONE) {
          line.who = g.kinds[kind].name;
          if(line.face < 0) line.face = g.kinds[kind].sprites[0];
        }
        if(!text(l, "text", lp, line.text, true)) return false;
        if(!count_text(line.text, at(lp, "text"))) return false;
        if(!event_keys(l, lp, line.event)) return false;

        Ref choices;
        if(!array(l, "choices", lp, choices, MAX_CHOICES)) return false;
        int c = 0;
        for(auto ch : choices.items()) {
          std::string cp = at(at(lp, "choices"), c++);
          Choice choice;
          if(!text(ch, "text", cp, choice.text, true)) return false;
          if(!event_keys(ch, cp, choice.event)) return false;
          if(Ref go = ch.get("goto")) {
            std::string label = go.string();
            for(size_t k = 0; k < dlg.lines.size(); k++)
              if(!dlg.lines[k].label.empty() && dlg.lines[k].label == label) choice.next = k;
            if(choice.next == NONE) return fail(at(cp, "goto"), "no line \"" + label + "\"");
          }
          line.choices.push_back(std::move(choice));
        }
      }
    }
    return true;
  }

  bool quests(Ref root) {
    int i = 0;
    for(auto q : root.get("quests").items()) {
      std::string p = at("quests", i);
      Quest &quest = g.quests[i++];
      if(!text(q, "title", p, quest.title)) return false;
      if(quest.title.empty()) quest.title = quest.id;
      Ref stages;
      if(!array(q, "stages", p, stages, MAX_STAGES)) return false;
      if(!stages || stages.size() == 0) return fail(at(p, "stages"), "missing");
      int n = 0;
      for(auto s : stages.items()) {
        std::string sp = at(at(p, "stages"), n++);
        Stage stage;
        if(!text(s, "log", sp, stage.log)) return false;
        if(!count_text(stage.log, at(sp, "log"))) return false;
        if(Ref goal = s.get("goal")) {
          std::string gp = at(sp, "goal");
          Goal &gl = stage.goal;
          if(goal.get("talk")) {
            gl.type = Goal::Type::talk;
            if(!ref(goal, "talk", gp, g.kinds, "kind", gl.target)) return false;
          } else if(goal.get("collect")) {
            gl.type = Goal::Type::collect;
            if(!ref(goal, "collect", gp, g.items, "item", gl.target)) return false;
          } else if(goal.get("defeat")) {
            gl.type = Goal::Type::defeat;
            if(!ref(goal, "defeat", gp, g.kinds, "kind", gl.target)) return false;
          } else if(goal.get("reach")) {
            gl.type = Goal::Type::reach;
            if(!ref(goal, "reach", gp, g.rooms, "room", gl.target)) return false;
            if(!spot(goal, gp, g.rooms[gl.target], "x", "y", gl.x, gl.y)) return false;
          } else if(Ref f = goal.get("flag")) {
            gl.type = Goal::Type::flag;
            if(!name(f, at(gp, "flag"), g.flags, MAX_FLAGS, gl.target)) return false;
          } else {
            return fail(gp, "one of talk, collect, defeat, reach, flag");
          }
          if(!number(goal, "count", gp, 1, 999, gl.count)) return false;
        }
        if(!event(s, "done", sp, stage.done)) return false;
        quest.stages.push_back(std::move(stage));
      }
    }
    return true;
  }

  bool songs(Ref root) {
    int i = 0;
    for(auto s : root.get("songs").items()) {
      std::string p = at("songs", i);
      Song &song = g.songs[i++];
      if(!number(s, "step_ms", p, 30, 1000, song.step_ms)) return false;
      if(!text(s, "lead", p, song.lead)) return false;
      if(!text(s, "bass", p, song.bass)) return false;
      if(!text(s, "drums", p, song.drums)) return false;
    }
    return true;
  }

  bool all(Ref root) {
    if(!root.is_object()) return fail("", "the file must be one JSON object");
    return header(root) && collect_ids(root) && stats(root) && tiles(root) && songs(root) && rooms(root) &&
           start(root) && kinds(root) && items(root) && dialogue(root) && quests(root) && event(root, "lose", "", g.lose);
  }
};

}

std::string load(std::string_view text, Game &game) {
  game = Game();
  json::Doc doc;
  std::string err = doc.parse(text);
  if(!err.empty()) return err;
  Loader loader{game, {}};
  if(!loader.all(doc.root())) {
    game = Game();
    return loader.error;
  }
  game.hash = fnv1a(text);
  return "";
}

const char *sound_name(Sound s) {
  return s < Sound::count ? sound_names[int(s)] : "";
}

}

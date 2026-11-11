// The world: rooms, turns, movement, events, dialogue, quests and drawing.
// Combat is in combat.cpp, enemy brains in ai.cpp, the bag in items.cpp and
// saves in save.cpp.

#include "chili/rpg/world.hpp"

#include "chili/input.hpp"
#include "chili/ui.hpp"

#include <algorithm>
#include <cstdio>

using namespace blit;

namespace chili::rpg {

namespace {

constexpr int TILE = 8;
constexpr uint16_t FADE_MS = 150;

const int DX[4] = {0, 0, -1, 1};  // by Dir
const int DY[4] = {1, -1, 0, 0};

Dir dir_of(int dx, int dy) {
  if(dx < 0) return Dir::left;
  if(dx > 0) return Dir::right;
  if(dy < 0) return Dir::up;
  return Dir::down;
}

using SfxTable = audio::Sfx[int(Sound::count)];
const SfxTable &default_sfx() {
  static const SfxTable table = {
    {audio::SFX1, Waveform::NOISE, 0x1400, 0x7fff, 20, 1, {{200, 20}}},                       // step
    {audio::SFX1, Waveform::SQUARE, 0x2000, 0x7fff, 30, 1, {{110, 40}}},                      // bump
    {audio::SFX2, Waveform::NOISE, 0x4000, 0x7fff, 60, 2, {{900, 30}, {400, 50}}, 1},         // hit
    {audio::SFX2, Waveform::SAW, 0x4800, 0x7fff, 80, 2, {{300, 50}, {180, 80}}, 2},           // hurt
    {audio::SFX2, Waveform::SQUARE, 0x2000, 0x7fff, 40, 1, {{220, 40}}},                      // miss
    {audio::SFX3, Waveform::SQUARE, 0x3000, 0x4000, 50, 3, {{660, 30}, {880, 30}, {1320, 60}}, 1},  // pickup
    {audio::SFX3, Waveform::TRIANGLE, 0x4000, 0x7fff, 60, 2, {{330, 60}, {494, 90}}},         // door
    {audio::SFX3, Waveform::SQUARE, 0x3800, 0x4000, 80, 4, {{523, 70}, {659, 70}, {784, 70}, {1047, 140}}, 3},  // level
    {audio::SFX3, Waveform::TRIANGLE, 0x4000, 0x7fff, 80, 3, {{784, 60}, {988, 60}, {1175, 120}}, 2},  // save
    {audio::SFX3, Waveform::SQUARE, 0x3800, 0x4000, 80, 3, {{659, 80}, {784, 80}, {1047, 160}}, 3},  // quest
    {audio::SFX3, Waveform::SQUARE, 0x3800, 0x4000, 120, 4, {{523, 100}, {659, 100}, {784, 100}, {1047, 300}}, 4},  // win
    {audio::SFX3, Waveform::SAW, 0x4000, 0x7fff, 160, 3, {{392, 150}, {311, 150}, {262, 400}}, 4},  // lose
  };
  return table;
}

}

World::World() {
  actors.reserve(MAX_ACTORS);
}

// ---------------------------------------------------------------------------
// Loading and rooms

std::string World::load(std::string_view text) {
  std::string err = rpg::load(text, game);
  if(!err.empty()) return err;
  songs.clear();
  for(auto &s : game.songs) songs.push_back(audio::Song{s.step_ms, true, s.lead.c_str(), s.bass.c_str(), s.drums.c_str()});
  new_game();
  return "";
}

void World::new_game() {
  state = State();
  for(size_t i = 0; i < game.stats.size(); i++) state.stats[i] = game.stats[i].start;
  player = Actor();
  player.stats = state.stats;
  result_ = Result::playing;
  screen_ = Screen::none;
  box.close();
  talk = NONE;
  song = NONE;
  warp_room = NONE;
  fade_ms = 0;
  floats.clear();
  particles.clear();
  shake.stop();
  if(!game.rooms.empty()) enter_room(game.start_room, game.start_x, game.start_y);
}

void World::enter_room(Index r, int x, int y) {
  state.room = r;
  player.x = x;
  player.y = y;
  player.ox = player.oy = 0;
  player.slide_ms = 0;
  actors.clear();
  const Room &rm = room();
  const RoomState &rs = state.rooms[r];
  for(size_t i = 0; i < rm.actors.size(); i++) {
    if(rs.gone & (1u << i)) continue;
    const RoomActor &ra = rm.actors[i];
    Actor a;
    a.kind = ra.kind;
    a.spawn = int8_t(i);
    a.x = a.home_x = ra.x;
    a.y = a.home_y = ra.y;
    a.stats = game.kinds[ra.kind].stats;
    actors.push_back(a);
  }
  particles.clear();
  floats.clear();
  play_room_music();
}

void World::play_room_music() {
  Index want = room().music;
  if(want == NONE || want == song) return;
  song = want;
  audio::play(songs[want]);
}

void World::warp(Index r, int x, int y, bool fade) {
  if(r < 0 || r >= Index(game.rooms.size())) return;
  if(!fade) {
    enter_room(r, x, y);
    return;
  }
  warp_room = r;
  warp_x = x;
  warp_y = y;
  fade_ms = FADE_MS;
  fading_in = false;
}

// ---------------------------------------------------------------------------
// Queries

bool World::flag(Index f) const {
  return f >= 0 && f < MAX_FLAGS && (state.flags[f / 32] >> (f % 32)) & 1;
}

void World::set_flag(Index f, bool on) {
  if(f < 0 || f >= MAX_FLAGS) return;
  if(on) state.flags[f / 32] |= 1u << (f % 32);
  else state.flags[f / 32] &= ~(1u << (f % 32));
}

const TileType &World::tile(int x, int y) const {
  return game.tiles[room().ground_at(x, y)];
}

bool World::blocked(int x, int y, bool sight) const {
  const Room &r = room();
  if(x < 0 || y < 0 || x >= r.w || y >= r.h) return true;
  const TileType &t = tile(x, y);
  uint8_t top = r.top_at(x, y);
  bool solid = t.solid || (top != Room::EMPTY && game.tiles[top].solid);
  return sight ? solid : solid || t.water;
}

Actor *World::actor_at(int x, int y) {
  if(player.x == x && player.y == y) return &player;
  for(auto &a : actors)
    if(a.alive && a.x == x && a.y == y) return &a;
  return nullptr;
}

bool World::busy() const {
  if(box.active() || screen_ != Screen::none || warp_room != NONE || fade_ms > 0 || result_ != Result::playing) return true;
  if(player.slide_ms) return true;
  for(auto &a : actors)
    if(a.slide_ms) return true;
  return false;
}

Point World::camera() const {
  const Room &r = room();
  int vw = blit::screen.bounds.w, vh = blit::screen.bounds.h;
  int px = player.x * TILE + player.ox * player.slide_ms / std::max<int>(game.step_ms, 1) + TILE / 2;
  int py = player.y * TILE + player.oy * player.slide_ms / std::max<int>(game.step_ms, 1) + TILE / 2;
  auto axis = [](int p, int view, int size) {
    if(size <= view) return -(view - size) / 2;  // a small room sits in the middle
    return std::clamp(p - view / 2, 0, size - view);
  };
  return Point(axis(px, vw, r.w * TILE), axis(py, vh, r.h * TILE));
}

// ---------------------------------------------------------------------------
// The turn

void World::update(uint32_t dt) {
  auto shrink = [dt](uint16_t &v) { v = v > dt ? v - dt : 0; };
  shrink(player.slide_ms);
  shrink(player.flash_ms);
  for(auto &a : actors) {
    shrink(a.slide_ms);
    shrink(a.flash_ms);
  }
  particles.update(dt);
  floats.update(dt);
  shake.update(dt);

  if(fade_ms) {
    shrink(fade_ms);
    if(!fade_ms && !fading_in && warp_room != NONE) {
      enter_room(warp_room, warp_x, warp_y);
      warp_room = NONE;
      fading_in = true;
      fade_ms = FADE_MS;
    } else if(!fade_ms) {
      fading_in = false;
    }
    return;
  }
  if(result_ != Result::playing) return;

  uint32_t pressed = buttons.pressed;
  if(box.active()) {
    if(box.update(dt, pressed)) next_line();
    return;
  }
  if(screen_ != Screen::none) {
    update_screen(pressed);
    return;
  }
  if(busy()) return;

  uint32_t held = buttons.state;
  int dx = 0, dy = 0;
  if(held & Button::DPAD_LEFT) dx = -1;
  else if(held & Button::DPAD_RIGHT) dx = 1;
  if(held & Button::DPAD_UP) dy = -1;
  else if(held & Button::DPAD_DOWN) dy = 1;
  if(!game.diagonal && dx && dy) dy = 0;

  if(dx || dy) {
    player_turn(dx, dy);
  } else if(pressed & Button::A) {
    // talk to whoever the player faces
    int f = int(player.facing);
    if(Actor *a = actor_at(player.x + DX[f], player.y + DY[f]); a && a != &player && game.kinds[a->kind].friendly)
      interact(*a);
  } else if(pressed & Button::B) {
    player_turn(0, 0);  // wait a turn
  } else if(pressed & Button::X) {
    open(Screen::inventory);
  } else if(pressed & Button::Y) {
    open(Screen::quests);
  }
}

void World::player_turn(int dx, int dy) {
  if(dx || dy) {
    player.facing = dir_of(dx, dy);
    int tx = player.x + dx, ty = player.y + dy;
    Actor *other = actor_at(tx, ty);
    if(other && other != &player) {
      if(game.kinds[other->kind].friendly) {
        interact(*other);
        return;  // talking takes no turn
      }
      attack(player, *other);
    } else if(!step(player, dx, dy)) {
      play(Sound::bump);
      return;  // walking into a wall takes no turn
    } else {
      play(Sound::step);
    }
  }
  if(result_ != Result::playing || warp_room != NONE) return;
  others_turn();
  end_turn();
}

void World::interact(Actor &a) {
  // a talk goal counts as soon as the player talks
  for(size_t q = 0; q < game.quests.size(); q++) {
    uint8_t st = state.quests[q];
    if(st == 0 || st == State::DONE || st > game.quests[q].stages.size()) continue;
    const Goal &g = game.quests[q].stages[st - 1].goal;
    if(g.type == Goal::Type::talk && g.target == a.kind) state.progress[q] = 1;
  }
  Index t = talk_of(a);
  Event e = bump_event(a);
  if(t != NONE) say(t);
  run(e, &a);
  if(!box.active()) check_quests();
}

Index World::talk_of(const Actor &a) const {
  if(a.spawn >= 0 && room().actors[a.spawn].talk != NONE) return room().actors[a.spawn].talk;
  return game.kinds[a.kind].talk;
}

Event World::bump_event(const Actor &a) const {
  if(a.spawn >= 0 && !room().actors[a.spawn].bump.empty()) return room().actors[a.spawn].bump;
  return game.kinds[a.kind].bump;
}

void World::slide(Actor &a, int dx, int dy, int px) {
  a.ox = -dx * px;
  a.oy = -dy * px;
  a.slide_ms = game.step_ms;
}

bool World::step(Actor &a, int dx, int dy) {
  if(dx || dy) a.facing = dir_of(dx, dy);
  int tx = a.x + dx, ty = a.y + dy;
  if(blocked(tx, ty) || actor_at(tx, ty)) return false;
  // no cutting corners past walls on a diagonal
  if(dx && dy && (blocked(a.x + dx, a.y) || blocked(a.x, a.y + dy))) return false;
  a.x = tx;
  a.y = ty;
  a.walk++;
  slide(a, dx, dy, TILE);
  after_step(a);
  return true;
}

void World::after_step(Actor &a) {
  const TileType &t = tile(a.x, a.y);
  if(t.damage) hurt(a, t.damage);
  if(!a.is_player() || !a.alive) return;

  const Room &r = room();
  RoomState &rs = state.rooms[state.room];
  for(size_t i = 0; i < r.items.size(); i++) {
    const RoomItem &it = r.items[i];
    if(rs.taken & (1u << i) || it.x != a.x || it.y != a.y) continue;
    if(give(it.item, it.count) == 0) {
      rs.taken |= 1u << i;
      play(Sound::pickup);
      popup(a.x, a.y, game.items[it.item].name, theme.gold);
    } else {
      popup(a.x, a.y, "BAG FULL", theme.dim);
    }
  }
  for(size_t i = 0; i < r.triggers.size(); i++) {
    const Trigger &tr = r.triggers[i];
    if(tr.x != a.x || tr.y != a.y || (tr.once && rs.fired & (1u << i))) continue;
    if(!check(tr.event) && !tr.event.else_count) continue;
    if(tr.once && check(tr.event)) rs.fired |= 1u << i;
    run(tr.event);
  }
  if(t.save) {
    save();
    play(Sound::save);
    popup(a.x, a.y, "SAVED", theme.text);
  }
  for(auto &w : r.warps)
    if(w.x == a.x && w.y == a.y) {
      play(Sound::door);
      warp(w.room, w.tx, w.ty);
    }
}

void World::others_turn() {
  for(size_t i = 0; i < actors.size(); i++) {
    Actor &a = actors[i];
    if(!a.alive || state.turn % game.kinds[a.kind].speed) continue;
    think(a);
    if(result_ != Result::playing || !player.alive) break;
  }
}

void World::end_turn() {
  state.turn++;
  check_quests();
}

// ---------------------------------------------------------------------------
// Events

bool World::check(const Event &e) const {
  for(int i = 0; i < e.if_count; i++) {
    const Condition &c = game.conditions[e.if_at + i];
    int have = 0;
    using T = Condition::Type;
    switch(c.type) {
      case T::flag: if(!flag(c.target)) return false; continue;
      case T::not_flag: if(flag(c.target)) return false; continue;
      case T::var: have = var(c.target); break;
      case T::has: have = count(c.target); break;
      case T::quest: have = state.quests[c.target]; break;
      case T::quest_done: have = state.quests[c.target] == State::DONE; break;
      case T::stat: have = stat(player, c.target); break;
    }
    bool ok = false;
    switch(c.op) {
      case Condition::Op::eq: ok = have == c.value; break;
      case Condition::Op::ne: ok = have != c.value; break;
      case Condition::Op::lt: ok = have < c.value; break;
      case Condition::Op::le: ok = have <= c.value; break;
      case Condition::Op::gt: ok = have > c.value; break;
      case Condition::Op::ge: ok = have >= c.value; break;
    }
    if(!ok) return false;
  }
  return true;
}

void World::run(const Event &e, Actor *self) {
  bool pass = check(e);
  uint16_t at = pass ? e.do_at : e.else_at;
  uint16_t n = pass ? e.do_count : e.else_count;
  for(uint16_t i = 0; i < n; i++) run_action(game.actions[at + i], self);
}

void World::run_action(const Action &a, Actor *self) {
  using T = Action::Type;
  switch(a.type) {
    case T::set: set_flag(a.target, true); break;
    case T::clear: set_flag(a.target, false); break;
    case T::add: state.vars[a.target] = std::clamp(state.vars[a.target] + a.value, -32768, 32767); break;
    case T::give:
      if(give(a.target, a.value) < a.value) {
        play(Sound::pickup);
        popup(player.x, player.y, game.items[a.target].name, theme.gold);
      }
      break;
    case T::take: take(a.target, a.value); break;
    case T::heal: heal(a.value); break;
    case T::hurt: hurt(player, a.value); break;
    case T::stat: player.stats[a.target] += a.value; break;
    case T::xp: gain_xp(a.value); break;
    case T::warp: warp(a.target, a.x, a.y); break;
    case T::say: say(a.target); break;
    case T::quest: set_quest(a.target, a.value); break;
    case T::quest_done: finish_quest(a.target); break;
    case T::sound: play(Sound(a.target)); break;
    case T::music:
      song = a.target;
      audio::play(songs[a.target]);
      break;
    case T::save: save(); break;
    case T::end: end(a.value == 1); break;
    case T::remove:
      if(self && !self->is_player()) {
        self->alive = false;
        if(self->spawn >= 0) state.rooms[state.room].gone |= 1u << self->spawn;
      }
      break;
    case T::call: {
      auto it = hooks.actions.find(game.calls[a.target]);
      if(it != hooks.actions.end() && it->second) it->second(*this);
      break;
    }
  }
}

void World::play(Sound s) {
  if(hooks.sound) {
    hooks.sound(s);
    return;
  }
  if(s < Sound::count) audio::play_sfx(default_sfx()[int(s)]);
}

void World::end(bool won) {
  result_ = won ? Result::won : Result::lost;
  box.close();
  screen_ = Screen::none;
  play(won ? Sound::win : Sound::lose);
}

void World::popup(int x, int y, std::string_view text, Pen color) {
  floats.add(Point(x * TILE + TILE / 2, y * TILE - 2), text, color);
}

// ---------------------------------------------------------------------------
// Dialogue

void World::say(Index d) {
  if(d < 0 || d >= Index(game.dialogue.size())) return;
  talk = d;
  line = -1;
  talk_gen++;
  next_line();
}

void World::next_line() {
  if(talk == NONE) return;
  const Dialogue &d = game.dialogue[talk];
  uint32_t gen = talk_gen;

  // the line just read: its actions, or the picked choice
  if(line >= 0 && line < int(d.lines.size())) {
    const Line &l = d.lines[line];
    run(l.event);
    if(gen != talk_gen) return;  // an action started other dialogue
    if(!l.choices.empty() && box.choice() >= 0) {
      const Choice &c = l.choices[box.choice()];
      run(c.event);
      if(gen != talk_gen) return;
      if(c.next == NONE) {
        talk = NONE;
        box.close();
        check_quests();
        return;
      }
      line = c.next - 1;
    }
  }

  // the next line whose conditions pass
  for(line++; line < int(d.lines.size()); line++) {
    const Line &l = d.lines[line];
    if(check(l.event)) break;
    if(l.event.else_count) run(l.event);
    if(gen != talk_gen) return;
  }
  if(line >= int(d.lines.size())) {
    talk = NONE;
    box.close();
    check_quests();
    return;
  }
  const Line &l = d.lines[line];
  box.say(l.text, l.who, l.face, sheet);
  if(!l.choices.empty()) {
    std::vector<std::string> texts;
    for(auto &c : l.choices) texts.push_back(c.text);
    box.ask(texts);
  }
}

// ---------------------------------------------------------------------------
// Quests

void World::set_quest(Index q, int stage) {
  if(q < 0 || q >= Index(game.quests.size())) return;
  int stages = game.quests[q].stages.size();
  bool started = state.quests[q] == 0 && stage > 0;
  state.quests[q] = stage > stages ? State::DONE : stage;
  state.progress[q] = 0;
  if(started) popup(player.x, player.y, "NEW QUEST", theme.accent);
}

void World::finish_quest(Index q) {
  if(q < 0 || q >= Index(game.quests.size()) || state.quests[q] == State::DONE) return;
  state.quests[q] = State::DONE;
  play(Sound::quest);
  popup(player.x, player.y, "QUEST DONE", theme.gold);
}

void World::check_quests() {
  // a stage's reward can finish the next stage too, so go round a few times
  for(int round = 0; round < MAX_STAGES; round++) {
    bool moved = false;
    for(size_t q = 0; q < game.quests.size(); q++) {
      uint8_t st = state.quests[q];
      const Quest &quest = game.quests[q];
      if(st == 0 || st == State::DONE || st > quest.stages.size()) continue;
      const Stage &stage = quest.stages[st - 1];
      const Goal &g = stage.goal;
      bool met = false;
      switch(g.type) {
        case Goal::Type::none: break;
        case Goal::Type::talk: met = state.progress[q] > 0; break;
        case Goal::Type::collect: met = count(g.target) >= g.count; break;
        case Goal::Type::defeat: met = state.progress[q] >= g.count; break;
        case Goal::Type::reach: met = state.room == g.target && player.x == g.x && player.y == g.y; break;
        case Goal::Type::flag: met = flag(g.target); break;
      }
      if(!met) continue;
      moved = true;
      if(st == quest.stages.size()) {
        finish_quest(q);
      } else {
        state.quests[q] = st + 1;
        state.progress[q] = 0;
        play(Sound::quest);
      }
      run(stage.done);
    }
    if(!moved) break;
  }
}

// ---------------------------------------------------------------------------
// Drawing

void World::draw_sprite(int sprite, Point at, bool flip) const {
  if(sheet) {
    blit::screen.sprites = sheet;
    blit::screen.sprite(uint16_t(sprite), at, flip ? SpriteTransform::HORIZONTAL : 0);
    return;
  }
  // no sheet: a square in a colour picked from the sprite number
  blit::screen.pen = Pen(60 + (sprite * 37) % 180, 60 + (sprite * 91) % 180, 60 + (sprite * 53) % 180);
  blit::screen.rectangle(Rect(at.x + 1, at.y + 1, TILE - 2, TILE - 2));
}

void World::render(uint32_t time) {
  auto &scr = blit::screen;
  scr.pen = Pen(game.background[0], game.background[1], game.background[2]);
  scr.clear();
  if(game.rooms.empty()) return;

  const Room &r = room();
  Point cam = camera() - shake.offset();
  int x0 = std::max<int>(0, cam.x / TILE), y0 = std::max<int>(0, cam.y / TILE);
  int x1 = std::min<int>(r.w, (cam.x + scr.bounds.w) / TILE + 1);
  int y1 = std::min<int>(r.h, (cam.y + scr.bounds.h) / TILE + 1);

  auto draw_tile = [&](uint8_t t, Point at) {
    if(hooks.draw_tile) {
      hooks.draw_tile(*this, t, at);
      return;
    }
    const TileType &tt = game.tiles[t];
    draw_sprite(tt.frames[(time / tt.frame_ms) % tt.frame_count], at);
  };

  for(int y = y0; y < y1; y++)
    for(int x = x0; x < x1; x++) draw_tile(r.ground_at(x, y), Point(x * TILE, y * TILE) - cam);

  const RoomState &rs = state.rooms[state.room];
  for(size_t i = 0; i < r.items.size(); i++)
    if(!(rs.taken & (1u << i))) draw_sprite(game.items[r.items[i].item].icon, Point(r.items[i].x * TILE, r.items[i].y * TILE) - cam);

  auto draw_actor = [&](const Actor &a, const uint8_t *sprites, bool flip_left, uint8_t frames) {
    if(!a.alive || (a.flash_ms && (a.flash_ms / 50) % 2)) return;
    int f = int(a.facing);
    bool flip = f == int(Dir::left) && flip_left;
    int sprite = flip ? sprites[int(Dir::right)] : sprites[f];
    sprite += a.walk % frames;
    int div = std::max<int>(game.step_ms, 1);
    Point at(a.x * TILE + a.ox * a.slide_ms / div, a.y * TILE + a.oy * a.slide_ms / div);
    draw_sprite(sprite, at - cam, flip);
  };
  for(auto &a : actors) {
    const Kind &k = game.kinds[a.kind];
    draw_actor(a, k.sprites, k.flip_left, k.frames);
  }
  draw_actor(player, game.player_sprites, game.player_flip, game.player_frames);

  if(!r.top.empty())
    for(int y = y0; y < y1; y++)
      for(int x = x0; x < x1; x++)
        if(uint8_t t = r.top_at(x, y); t != Room::EMPTY) draw_tile(t, Point(x * TILE, y * TILE) - cam);

  particles.render(Point(0, 0) - cam);
  floats.render(Point(0, 0) - cam);

  if(show_hud) draw_hud();
  box.render(time);
  if(screen_ == Screen::inventory) draw_inventory(time);
  if(screen_ == Screen::quests) draw_quests(time);

  if(fade_ms) {
    int a = fading_in ? fade_ms * 255 / FADE_MS : 255 - fade_ms * 255 / FADE_MS;
    fx::dim(uint8_t(std::clamp(a, 0, 255)));
  }
}

}

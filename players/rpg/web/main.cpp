// The RPG web player's C API, for player.js. See docs/rpg/players.md.
//
// It runs chili::rpg::World on a small stand-in for the 32blit firmware: a
// 128x128 screen in memory, buttons and a clock from JavaScript, the 32blit
// sound channels mixed into samples, and no files (so no saves).

#include "32blit.hpp"
#include "engine/api_private.hpp"

#include "chili/audio.hpp"
#include "chili/rpg/world.hpp"

#include <emscripten/emscripten.h>

#include <cstdio>
#include <string>
#include <vector>

using namespace blit;
namespace rpg = chili::rpg;

// ---------------------------------------------------------------------------
// The stand-in firmware

namespace {

AudioChannel channel_data[CHANNEL_COUNT];
uint8_t screen_rgb[128 * 128 * 3];
uint32_t clock_ms = 0;
uint32_t seed = 1;

uint32_t now() { return clock_ms; }
uint32_t random() {
  seed = seed * 1103515245u + 12345u;
  return seed >> 8;
}
void debug(const char *) {}
bool set_screen_mode_format(ScreenMode, SurfaceTemplate &t) {
  t.data = screen_rgb;
  t.bounds = Size(128, 128);
  t.format = PixelFormat::RGB;
  return true;
}
// No files: the browser always starts a game fresh.
void *open_file(const std::string &, int) { return nullptr; }
int32_t read_file(void *, uint32_t, uint32_t, char *) { return -1; }
int32_t write_file(void *, uint32_t, uint32_t, const char *) { return -1; }
int32_t close_file(void *) { return 0; }
uint32_t get_file_length(void *) { return 0; }
bool file_exists(const std::string &) { return false; }
const char *get_save_path() { return ""; }
const char *get_launch_path() { return ""; }

APIConst make_api() {
  APIConst a{};
  a.channels = channel_data;
  a.now = now;
  a.random = random;
  a.debug = debug;
  a.set_screen_mode_format = set_screen_mode_format;
  a.open_file = open_file;
  a.read_file = read_file;
  a.write_file = write_file;
  a.close_file = close_file;
  a.get_file_length = get_file_length;
  a.file_exists = file_exists;
  a.get_save_path = get_save_path;
  a.get_launch_path = get_launch_path;
  return a;
}

APIConst api_const = make_api();
APIData api_data_;

}  // namespace

namespace blit {
const APIConst &api = api_const;
APIData &api_data = api_data_;
}  // namespace blit

// ---------------------------------------------------------------------------
// The game

namespace {

rpg::World world;
bool loaded = false;
std::string error;
std::string debug_json;
uint8_t rgba[128 * 128 * 4];
std::vector<float> samples;
uint32_t steps = 0;
uint32_t held = 0;

const chili::audio::Sfx sfx_talk = {chili::audio::SFX4, Waveform::SQUARE, 0x1800, 0x7fff, 10, 1, {{520, 12}}};

void json_string(std::string &out, std::string_view s) {
  out += '"';
  for(unsigned char c : s) {
    if(c == '"' || c == '\\') {
      out += '\\';
      out += char(c);
    } else if(c < 0x20) {
      char buf[8];
      snprintf(buf, sizeof buf, "\\u%04x", c);
      out += buf;
    } else {
      out += char(c);
    }
  }
  out += '"';
}

void hold(uint32_t buttons) {
  uint32_t before = api_data_.buttons.state;
  api_data_.buttons.state = buttons;
  api_data_.buttons.pressed = buttons & ~before;
  api_data_.buttons.released = before & ~buttons;
}

// JavaScript's button mask to 32blit's.
uint32_t blit_buttons(int mask) {
  uint32_t b = 0;
  if(mask & 1) b |= Button::DPAD_UP;
  if(mask & 2) b |= Button::DPAD_DOWN;
  if(mask & 4) b |= Button::DPAD_LEFT;
  if(mask & 8) b |= Button::DPAD_RIGHT;
  if(mask & 16) b |= Button::A;
  if(mask & 32) b |= Button::B;
  if(mask & 64) b |= Button::X;
  if(mask & 128) b |= Button::Y;
  return b;
}

}  // namespace

extern "C" {

// Loads a game from .rpg text. Returns 1, or 0 and sets rpg_error().
EMSCRIPTEN_KEEPALIVE int rpg_load(const char *text) {
  set_screen_mode(ScreenMode::hires);
  chili::audio::init();
  for(auto &c : channel_data) c.off();
  world.sheet = nullptr;
  error = world.load(text);
  loaded = error.empty();
  if(loaded) world.text_box().on_letter = [] { chili::audio::play_sfx(sfx_talk); };
  return loaded;
}

EMSCRIPTEN_KEEPALIVE const char *rpg_error() { return error.c_str(); }

// One step of 1/60 s. buttons: up 1, down 2, left 4, right 8, A 16, B 32, X 64, Y 128.
EMSCRIPTEN_KEEPALIVE void rpg_step(int buttons) {
  if(!loaded) return;
  hold(blit_buttons(buttons));
  steps++;
  uint32_t next = steps * 1000 / 60;
  uint32_t dt = next - clock_ms;
  clock_ms = next;
  world.update(dt);
  chili::audio::update(clock_ms);
}

// Starts the game again from the beginning.
EMSCRIPTEN_KEEPALIVE void rpg_restart() {
  if(!loaded) return;
  for(auto &c : channel_data) c.off();
  world.new_game();
}

// The screen now, as 128 x 128 RGBA.
EMSCRIPTEN_KEEPALIVE const uint8_t *rpg_rgba() {
  if(loaded) world.render(clock_ms);
  for(int i = 0; i < 128 * 128; i++) {
    rgba[i * 4 + 0] = screen_rgb[i * 3 + 0];
    rgba[i * 4 + 1] = screen_rgb[i * 3 + 1];
    rgba[i * 4 + 2] = screen_rgb[i * 3 + 2];
    rgba[i * 4 + 3] = 255;
  }
  return rgba;
}

// The next `count` sound samples, -1 to 1, at 22050 Hz.
EMSCRIPTEN_KEEPALIVE const float *rpg_audio(int count) {
  samples.resize(count);
  for(int i = 0; i < count; i++) samples[i] = (int(get_audio_frame()) - 0x8000) / 32768.0f;
  return samples.data();
}

// What is happening, as JSON. See docs/rpg/players.md.
EMSCRIPTEN_KEEPALIVE const char *rpg_debug() {
  std::string &o = debug_json;
  o.clear();
  if(!loaded) return "{\"state\":\"none\"}";
  const rpg::Game &g = world.game;
  const rpg::State &s = world.state;
  const char *states[] = {"playing", "won", "lost"};
  o += "{\"state\":\"";
  o += states[int(world.result())];
  o += "\",\"title\":";
  json_string(o, g.title);
  o += ",\"room\":";
  json_string(o, g.rooms[s.room].id);
  o += ",\"x\":" + std::to_string(world.player.x) + ",\"y\":" + std::to_string(world.player.y);
  o += ",\"level\":" + std::to_string(s.level) + ",\"xp\":" + std::to_string(s.xp);
  o += ",\"turn\":" + std::to_string(s.turn);
  o += ",\"stats\":{";
  for(size_t i = 0; i < g.stats.size(); i++) {
    if(i) o += ',';
    json_string(o, g.stats[i].id);
    o += ':' + std::to_string(world.stat(world.player, rpg::Index(i)));
  }
  o += "},\"hp\":" + std::to_string(world.player.stats[g.hp]);
  o += ",\"talking\":";
  o += world.talking() ? "true" : "false";
  o += ",\"speaker\":";
  json_string(o, world.talking() ? world.text_box().speaker() : "");
  o += ",\"text\":";
  json_string(o, world.talking() ? world.text_box().text() : "");
  o += ",\"choices\":[";
  if(world.talking()) {
    const auto &c = world.text_box().choices();
    for(size_t i = 0; i < c.size(); i++) {
      if(i) o += ',';
      json_string(o, c[i]);
    }
  }
  o += "],\"flags\":[";
  bool first = true;
  for(size_t i = 0; i < g.flags.size(); i++) {
    if(!world.flag(rpg::Index(i))) continue;
    if(!first) o += ',';
    first = false;
    json_string(o, g.flags[i]);
  }
  o += "],\"vars\":{";
  for(size_t i = 0; i < g.vars.size(); i++) {
    if(i) o += ',';
    json_string(o, g.vars[i]);
    o += ':' + std::to_string(world.var(rpg::Index(i)));
  }
  o += "},\"quests\":{";
  for(size_t i = 0; i < g.quests.size(); i++) {
    if(i) o += ',';
    json_string(o, g.quests[i].id);
    o += ':';
    o += s.quests[i] == rpg::State::DONE ? "\"done\"" : std::to_string(s.quests[i]);
  }
  o += "},\"bag\":[";
  first = true;
  for(const auto &slot : s.bag) {
    if(slot.item == rpg::NONE) continue;
    if(!first) o += ',';
    first = false;
    o += "{\"item\":";
    json_string(o, g.items[slot.item].id);
    o += ",\"count\":" + std::to_string(slot.count);
    o += ",\"equipped\":";
    o += slot.equipped ? "true" : "false";
    o += '}';
  }
  o += "]}";
  return o.c_str();
}

}  // extern "C"

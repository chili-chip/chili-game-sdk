// The RPG player app: a handler for the chilichip console that plays any
// .rpg game file. The home screen lists the .rpg files on the drive and starts
// this app with the one you pick (get_launch_path()). Started without a file,
// it plays Pepper Village, which is built in.
//
// The game itself is chili::rpg::World. This file adds the title menu, the
// pause menu, the end screen and the screen that shows a broken file's error.

#include "32blit.hpp"

#include "assets.hpp"
#include "chili/audio.hpp"
#include "chili/fx.hpp"
#include "chili/menu.hpp"
#include "chili/rpg/world.hpp"
#include "chili/scene.hpp"
#include "chili/screens.hpp"

#include <string>
#include <string_view>

using namespace blit;
namespace audio = chili::audio;
namespace fx = chili::fx;
namespace rpg = chili::rpg;

rpg::World world;
chili::Scenes scenes;
std::string error;     // why the game could not be opened
bool playing = false;  // in the game, not on the title or end screen

const audio::Sfx sfx_move = {audio::SFX4, Waveform::SQUARE, 0x2800, 0x7fff, 20, 1, {{880, 30}}};
const audio::Sfx sfx_select = {audio::SFX4, Waveform::SQUARE, 0x3000, 0x7fff, 40, 2, {{660, 40}, {990, 70}}};
const audio::Sfx sfx_back = {audio::SFX4, Waveform::SQUARE, 0x3000, 0x7fff, 40, 2, {{660, 40}, {440, 70}}};
const audio::Sfx sfx_talk = {audio::SFX4, Waveform::SQUARE, 0x1800, 0x7fff, 10, 1, {{520, 12}}};

void ui_sound(chili::UiSound s) {
  switch(s) {
    case chili::UiSound::move: audio::play_sfx(sfx_move); break;
    case chili::UiSound::change:
    case chili::UiSound::select: audio::play_sfx(sfx_select); break;
    case chili::UiSound::back: audio::play_sfx(sfx_back); break;
  }
}

// Loads the game the app was started with, or the built-in one. Returns ""
// or what is wrong.
std::string open_game() {
  const char *path = get_launch_path();
  if(!path || !path[0])
    return world.load(std::string_view(reinterpret_cast<const char *>(asset_template_rpg), asset_template_rpg_length));

  File file(path);
  if(!file.is_open()) return std::string("Can't read ") + path;
  std::string text(file.get_length(), '\0');
  file.read(0, text.size(), text.data());
  return world.load(text);
}

// The biggest letters the title fits in, 3 to 1, or 0 for plain text.
int title_scale(std::string_view title) {
  int w = screen.measure_text(title, minimal_font, true).w;
  for(int scale = 3; scale >= 1; scale--)
    if(w * scale <= 120) return scale;
  return 0;
}

// The title and end screens show the start room behind a dark veil.
void backdrop(uint32_t time) {
  world.show_hud = false;
  world.render(time);
  world.show_hud = true;
  fx::dim(150);
}

void go_title();
void go_play(bool from_save);

struct Help : chili::Scene {
  chili::TextScreen page{"HOW TO PLAY", {"D-PAD  walk, bump to fight", "A      pick up, talk", "B      wait a turn", "X      bag", "Y      quests", "MENU   pause"}};
  void update(uint32_t) override {
    if(page.update(scenes.pressed())) go_title();
  }
  void render(uint32_t time) override {
    backdrop(time);
    page.render(time);
  }
} help;

struct Title : chili::Scene {
  chili::Menu menu{{
    chili::Menu::action("CONTINUE", [] { go_play(true); }),
    chili::Menu::action("NEW GAME", [] { go_play(false); }),
    chili::Menu::action("HOW TO PLAY", [] { scenes.go(help); }),
  }};
  void enter() override {
    world.new_game();
    menu.select(world.has_save() ? 0 : 1);
  }
  void update(uint32_t) override {
    uint32_t pressed = scenes.pressed();
    // CONTINUE only works with a save
    if(menu.selected() == 0 && !world.has_save()) menu.select(1);
    menu.update(pressed);
    if(menu.selected() == 0 && !world.has_save()) menu.select(pressed & Button::DPAD_UP ? 2 : 1);
  }
  void render(uint32_t time) override {
    backdrop(time);
    const std::string &name = world.game.title;
    int scale = title_scale(name);
    if(scale) fx::big_text(name, Point(64, 28), scale, chili::theme.accent);
    else screen.pen = chili::theme.accent, screen.text(name, minimal_font, Rect(8, 14, 112, 30), true, TextAlign::center_center);
    if(!world.game.author.empty())
      fx::text("BY " + world.game.author, Point(64, 46), chili::theme.dim, TextAlign::top_center);
    menu.render(Rect(24, 62, 80, 34));
    fx::text("CHILI RPG", Point(64, 118), chili::theme.dim, TextAlign::top_center);
  }
} title;

struct Play : chili::Scene {
  bool paused = false;
  chili::Menu pause_menu{{
    chili::Menu::action("RESUME", [] { resume(); }),
    chili::Menu::action("SAVE", [] {
      world.save();
      world.popup(world.player.x, world.player.y, "SAVED", chili::theme.text);
      resume();
    }),
    chili::Menu::action("QUIT", [] { go_title(); }),
  }};
  static void resume();

  Play() { pause_menu.on_back = [] { resume(); }; }

  void update(uint32_t dt) override {
    if(paused) {
      pause_menu.update(scenes.pressed());
      return;
    }
    if(scenes.pressed() & Button::MENU && !world.busy()) {
      paused = true;
      pause_menu.select(0);
      chili::play_ui_sound(chili::UiSound::select);
      return;
    }
    world.update(dt);
    if(world.result() != rpg::Result::playing) {
      playing = false;
      scenes.go(ending, 600);
    }
  }

  void render(uint32_t time) override {
    world.render(time);
    if(paused) {
      fx::dim(140);
      fx::panel(Rect(24, 36, 80, 58));
      fx::big_text("PAUSED", Point(64, 47), 2, chili::theme.text);
      pause_menu.render(Rect(30, 60, 68, 30));
    }
  }

  struct Ending : chili::Scene {
    void update(uint32_t) override {
      if(scenes.pressed() & (Button::A | Button::B | Button::MENU)) go_title();
    }
    void render(uint32_t time) override {
      backdrop(time);
      bool won = world.result() == rpg::Result::won;
      fx::big_text(won ? "THE END" : "GAME OVER", Point(64, 40), 2, won ? chili::theme.gold : chili::theme.border);
      fx::text("LEVEL " + std::to_string(world.state.level) + "   TURNS " + std::to_string(world.state.turn), Point(64, 62),
               chili::theme.dim, TextAlign::top_center);
      if(scenes.time_in_scene() > scenes.guard_ms) fx::text("A: TITLE", Point(64, 104), chili::theme.text, TextAlign::top_center);
    }
  } ending;
} in_game;

void Play::resume() {
  in_game.paused = false;
}

void go_title() {
  playing = false;
  scenes.go(title, 300);
}

void go_play(bool from_save) {
  if(!from_save || !world.load_save()) world.new_game();
  in_game.paused = false;
  playing = true;
  scenes.go(in_game, 300);
}

void init() {
  set_screen_mode(ScreenMode::hires);
  chili::ui_sound = ui_sound;
  audio::init();

  playing = false;
  error = open_game();
  if(!error.empty()) return;

  world.text_box().on_letter = [] { audio::play_sfx(sfx_talk); };
  // Holding HOME or quitting from the system menu keeps the game.
  blit::on_quit = [] {
    if(playing) world.save();
  };

  scenes.guard_ms = 300;
  scenes.go(title);
}

void update(uint32_t time) {
  if(!error.empty()) return;
  scenes.update(time);
  audio::update(time);
}

void render(uint32_t time) {
  screen.alpha = 255;
  if(!error.empty()) {
    screen.pen = Pen(0, 0, 0);
    screen.clear();
    screen.pen = Pen(255, 255, 255);
    screen.text("Can't play this game:", minimal_font, Point(4, 4));
    screen.text(screen.wrap_text(error, screen.bounds.w - 8, minimal_font), minimal_font,
                Rect(4, 16, screen.bounds.w - 8, screen.bounds.h - 20));
    return;
  }
  scenes.render(time);
}

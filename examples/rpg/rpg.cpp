// Pepper Village: a small RPG made with the chili game SDK's RPG parts.
//
// All of the game (maps, people, enemies, items, dialogue and the quest) is
// in assets/pepper.rpg. This file only adds a title menu, a pause menu and
// the end screens around chili::rpg::World.
//
// Art: Micro Roguelike by Kenney (www.kenney.nl), CC0.

#include "32blit.hpp"

#include "assets.hpp"
#include "chili/audio.hpp"
#include "chili/fx.hpp"
#include "chili/menu.hpp"
#include "chili/rpg/world.hpp"
#include "chili/scene.hpp"
#include "chili/screens.hpp"

#include <string_view>

using namespace blit;
namespace audio = chili::audio;
namespace fx = chili::fx;
namespace rpg = chili::rpg;

rpg::World world;
chili::Scenes scenes;

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

// The title and end screens show the village behind a dark veil.
void backdrop(uint32_t time) {
  world.show_hud = false;
  world.render(time);
  world.show_hud = true;
  fx::dim(150);
}

void go_title();
void go_play(bool from_save);

struct Help : chili::Scene {
  chili::TextScreen page{"HOW TO PLAY", {"D-PAD  walk, bump to fight", "A      talk, next line", "B      wait a turn", "X      bag", "Y      quests", "MENU   pause"}};
  void update(uint32_t) override {
    if(page.update(scenes.pressed())) go_title();
  }
  void render(uint32_t time) override {
    backdrop(time);
    page.render(time);
  }
} help;

struct Credits : chili::Scene {
  chili::TextScreen page{"CREDITS", {"Made with the", "chili game SDK", "", "Art: Micro Roguelike", "by Kenney (CC0)", "www.kenney.nl"}};
  void update(uint32_t) override {
    if(page.update(scenes.pressed())) go_title();
  }
  void render(uint32_t time) override {
    backdrop(time);
    page.render(time);
  }
} credits;

struct Title : chili::Scene {
  chili::Menu menu{{
    chili::Menu::action("CONTINUE", [] { go_play(true); }),
    chili::Menu::action("NEW GAME", [] { go_play(false); }),
    chili::Menu::action("HOW TO PLAY", [] { scenes.go(help); }),
    chili::Menu::action("CREDITS", [] { scenes.go(credits); }),
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
    if(menu.selected() == 0 && !world.has_save()) menu.select(pressed & Button::DPAD_UP ? 3 : 1);
  }
  void render(uint32_t time) override {
    backdrop(time);
    fx::big_text("PEPPER", Point(64, 20), 3, chili::theme.accent);
    fx::big_text("VILLAGE", Point(64, 42), 2, chili::theme.text);
    menu.render(Rect(24, 62, 80, 44));
    fx::text("ART BY KENNEY", Point(64, 118), chili::theme.dim, TextAlign::top_center);
  }
} title;

struct Play : chili::Scene {
  bool paused = false;
  chili::Menu pause_menu{{
    chili::Menu::action("RESUME", [] { play_resume(); }),
    chili::Menu::action("SAVE", [] {
      world.save();
      world.popup(world.player.x, world.player.y, "SAVED", chili::theme.text);
      play_resume();
    }),
    chili::Menu::action("QUIT", [] { go_title(); }),
  }};
  static void play_resume();

  Play() { pause_menu.on_back = [] { play_resume(); }; }

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
    if(world.result() != rpg::Result::playing) scenes.go(ending, 600);
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
      fx::text(won ? "The sun stone is home." : "The cave was too dark.", Point(64, 60), chili::theme.text, TextAlign::top_center);
      fx::text("LEVEL " + std::to_string(world.state.level) + "   TURNS " + std::to_string(world.state.turn), Point(64, 72),
               chili::theme.dim, TextAlign::top_center);
      if(scenes.time_in_scene() > scenes.guard_ms) fx::text("A: TITLE", Point(64, 104), chili::theme.text, TextAlign::top_center);
    }
  } ending;
} play;

void Play::play_resume() {
  play.paused = false;
}

void go_title() { scenes.go(title, 300); }

void go_play(bool from_save) {
  if(!from_save || !world.load_save()) world.new_game();
  play.paused = false;
  scenes.go(play, 300);
}

void init() {
  set_screen_mode(ScreenMode::hires);
  chili::ui_sound = ui_sound;
  audio::init();

  world.sheet = Surface::load(asset_sheet);
  std::string err = world.load(std::string_view(reinterpret_cast<const char *>(asset_pepper_rpg), asset_pepper_rpg_length));
  if(!err.empty()) debug(("pepper.rpg: " + err + "\n").c_str());
  world.text_box().on_letter = [] { audio::play_sfx(sfx_talk); };

  scenes.guard_ms = 300;
  scenes.go(title);
}

void update(uint32_t time) {
  scenes.update(time);
  audio::update(time);
}

void render(uint32_t time) {
  screen.alpha = 255;
  scenes.render(time);
}

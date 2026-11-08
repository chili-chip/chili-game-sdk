// Chili Catch: move the bowl with left/right and catch the falling chillies.
// Three misses and the round is over. A small game that uses every module of
// the chili game SDK; the docs take their screenshots from it.

#include "32blit.hpp"

#include "chili/audio.hpp"
#include "chili/fx.hpp"
#include "chili/input.hpp"
#include "chili/menu.hpp"
#include "chili/save.hpp"
#include "chili/scene.hpp"
#include "chili/scores.hpp"
#include "chili/screens.hpp"

#include <cstring>
#include <string>

using namespace blit;
namespace audio = chili::audio;
namespace fx = chili::fx;

// ---------------------------------------------------------------------------
// Save

struct ScoreEntry {
  char name[4];
  uint32_t score;
};

struct SaveData {
  uint8_t speed = 1;
  uint8_t music = 1;
  uint8_t sound = 1;
  char last_name[4] = "AAA";
  chili::HighScores<ScoreEntry, 5> scores{};
};

chili::Save<SaveData> save(0x48435443, 1);  // "CTCH"

// ---------------------------------------------------------------------------
// Sound

const audio::Song title_song = {
  150, true,
  "A4 - C5 - E5 - C5 - D5 - - - B4 - - - A4 - C5 - E5 - A5 - G5 - - - E5 - - - ",
  "A2 - - - E3 - - - G2 - - - D3 - - - A2 - - - E3 - - - C3 - - - G2 - - - ",
  "K . H . S . H . K . H . S . H H K . H . S . H . K . H . S . S S ",
};

const audio::Song game_song = {
  120, true,
  "E5 - G5 - A5 - G5 - E5 - D5 - C5 - D5 - E5 - G5 - A5 - C6 - B5 - - - . . . . ",
  "A2 A3 A2 A3 F2 F3 F2 F3 C3 C4 C3 C4 G2 G3 G2 G3 A2 A3 A2 A3 F2 F3 F2 F3 E2 E3 E2 E3 E2 E3 G#2 B2 ",
  "K . H . S . H . K K H . S . H H K . H . S . H . K K H . S S S S ",
};

const audio::Song over_song = {130, false, "E5 - D5 - C5 - B4 - A4 - - - - - - - ", "C3 - - - B2 - - - A2 - - - - - - - ", "K . . . . . . . K . . . . . . . "};

const audio::Sfx sfx_move = {audio::SFX2, Waveform::SQUARE, 0x2800, 0x7fff, 20, 1, {{880, 30}}};
const audio::Sfx sfx_select = {audio::SFX2, Waveform::SQUARE, 0x3000, 0x7fff, 40, 2, {{660, 40}, {990, 70}}};
const audio::Sfx sfx_back = {audio::SFX2, Waveform::SQUARE, 0x3000, 0x7fff, 40, 2, {{660, 40}, {440, 70}}};
const audio::Sfx sfx_catch = {audio::SFX1, Waveform::SQUARE, 0x4000, 0x4000, 50, 3, {{523, 30}, {784, 30}, {1047, 70}}};
const audio::Sfx sfx_miss = {audio::SFX1, Waveform::SAW, 0x4800, 0x7fff, 120, 3, {{330, 60}, {247, 80}, {196, 120}}, 1};

void ui_sound(chili::UiSound s) {
  switch(s) {
    case chili::UiSound::move: audio::play_sfx(sfx_move); break;
    case chili::UiSound::change:
    case chili::UiSound::select: audio::play_sfx(sfx_select); break;
    case chili::UiSound::back: audio::play_sfx(sfx_back); break;
  }
}

void apply_settings() {
  audio::set_music(save.data.music);
  audio::set_sound(save.data.sound);
}

// ---------------------------------------------------------------------------
// Scenes

chili::Scenes scenes;
fx::Particles particles;
fx::FloatingText popups;
fx::Shake shake;

void draw_chilli(Point p, uint32_t time) {
  screen.pen = Pen(220, 40, 40);
  screen.rectangle(Rect(p.x - 2, p.y - 3, 4, 7));
  screen.pen = Pen(255, 120, 110);
  screen.pixel(Point(p.x - 1, p.y - 2));
  screen.pen = Pen(80, 190, 70);
  screen.rectangle(Rect(p.x - 1, p.y - 5, 2, 2));
}

void draw_sky(uint32_t time) {
  screen.pen = Pen(20, 18, 32);
  screen.clear();
  for(int i = 0; i < 12; i++) {
    int x = (i * 37 + 11) % 128;
    int y = (i * 53 + 7) % 100;
    screen.pen = ((time / 400 + i) % 3) ? Pen(70, 70, 100) : Pen(160, 160, 200);
    screen.pixel(Point(x, y));
  }
  screen.pen = Pen(60, 40, 30);
  screen.rectangle(Rect(0, 118, 128, 10));
}

// Scene changes, defined once every scene exists.
void go_title(uint32_t fade_ms = 0);
void go_play();
void go_results();
void go_over();

struct Help : chili::Scene {
  chili::TextScreen page{"HOW TO", {"Left/right moves", "the bowl.", "", "Catch the chillies.", "Three misses and", "it is over.", "", "MENU pauses."}};
  void update(uint32_t) override;
  void render(uint32_t time) override {
    draw_sky(time);
    page.render(time);
  }
} help;

struct Scores : chili::Scene {
  chili::ScoreTable table;
  void update(uint32_t) override;
  void render(uint32_t time) override {
    draw_sky(time);
    fx::big_text("HIGH SCORES", Point(64, 14), 1, chili::theme.gold);
    fx::panel(Rect(8, 26, 112, 56));
    table.render(save.data.scores, Rect(12, 31, 104, 45));
    fx::text("B: BACK", Point(64, 114), chili::theme.dim, TextAlign::top_center);
  }
} scores_scene;

struct Title : chili::Scene {
  chili::Menu menu{{
    chili::Menu::action("PLAY", [] { go_play(); }),
    chili::Menu::choice("SPEED", {"SLOW", "NORMAL", "FAST"}, save.data.speed, [] { save.store(); }),
    chili::Menu::toggle("MUSIC", save.data.music, [] { apply_settings(); save.store(); }),
    chili::Menu::toggle("SOUND", save.data.sound, [] { apply_settings(); save.store(); }),
    chili::Menu::action("HIGH SCORES", [] { scenes.go(scores_scene); }),
    chili::Menu::action("HOW TO PLAY", [] { scenes.go(help); }),
  }};
  void enter() override {
    if(audio::current() != &title_song) audio::play(title_song);
  }
  void update(uint32_t) override { menu.update(scenes.pressed()); }
  void render(uint32_t time) override {
    draw_sky(time);
    fx::big_text("CHILI", Point(64, 14), 3, chili::theme.accent);
    fx::big_text("CATCH", Point(64, 36), 2, chili::theme.text);
    menu.render(Rect(14, 52, 100, 60));
    const ScoreEntry &best = save.data.scores.best();
    if(best.score) {
      fx::text("BEST " + std::to_string(best.score) + " " + best.name, Point(64, 118), chili::theme.gold, TextAlign::top_center);
    }
  }
} title;

void Help::update(uint32_t) {
  if(page.update(scenes.pressed())) go_title();
}

void Scores::update(uint32_t) {
  if(scenes.pressed() & (Button::A | Button::B | Button::MENU)) {
    chili::play_ui_sound(chili::UiSound::back);
    go_title();
  }
}

uint32_t score = 0;
int new_rank = -1;

struct Play;
extern Play play;

struct Play : chili::Scene {
  float bowl = 64;
  float chilli_x = 64, chilli_y = -8;
  int misses = 0;
  bool paused = false;
  chili::Menu pause_menu{{
    chili::Menu::action("RESUME", [] { play.resume(); }),
    chili::Menu::action("RESTART", [] { scenes.go(play, 150); }),
    chili::Menu::action("QUIT", [] { go_title(150); }),
  }};

  Play() { pause_menu.on_back = [] { play.resume(); }; }

  void resume() {
    paused = false;
    audio::pause(false);
  }

  float fall_speed() const { return (0.04f + save.data.speed * 0.02f) * (1.0f + score / 1000.0f); }

  void drop() {
    chilli_x = 12 + blit::random() % 104;
    chilli_y = -8;
  }

  void enter() override {
    score = 0;
    misses = 0;
    paused = false;
    bowl = 64;
    drop();
    particles.clear();
    popups.clear();
    audio::play(game_song);
  }

  void update(uint32_t dt) override {
    uint32_t pressed = scenes.pressed();
    if(paused) {
      pause_menu.update(pressed);
      return;
    }
    if(pressed & Button::MENU) {
      paused = true;
      pause_menu.select(0);
      audio::pause(true);
      chili::play_ui_sound(chili::UiSound::select);
      return;
    }

    uint32_t held = scenes.held();
    if(held & Button::DPAD_LEFT) bowl -= 0.12f * dt;
    if(held & Button::DPAD_RIGHT) bowl += 0.12f * dt;
    bowl = std::max(10.0f, std::min(118.0f, bowl));

    chilli_y += fall_speed() * dt;
    if(chilli_y >= 108 && chilli_y < 116 && std::abs(chilli_x - bowl) < 11) {
      score += 10;
      particles.burst(Point(chilli_x, 108), Pen(220, 40, 40), 20);
      popups.add(Point(chilli_x, 100), "+10", chili::theme.gold);
      audio::play_sfx(sfx_catch);
      drop();
    } else if(chilli_y > 124) {
      misses++;
      shake.start(400);
      audio::play_sfx(sfx_miss);
      drop();
      if(misses >= 3) {
        new_rank = save.data.scores.rank(score);
        go_results();
      }
    }
  }

  void render(uint32_t time) override {
    draw_sky(time);
    Point o = shake.offset();
    draw_chilli(Point(chilli_x, chilli_y) + o, time);
    screen.pen = Pen(230, 200, 150);
    screen.rectangle(Rect(int(bowl) - 10 + o.x, 112 + o.y, 20, 4));
    screen.rectangle(Rect(int(bowl) - 8 + o.x, 116 + o.y, 16, 2));
    particles.render(o);
    popups.render(o);

    fx::text(std::to_string(score), Point(4, 3), chili::theme.text);
    for(int i = 0; i < 3; i++) {
      screen.pen = i < misses ? Pen(90, 40, 40) : Pen(220, 40, 40);
      screen.rectangle(Rect(108 + i * 6, 3, 4, 6));
    }

    if(paused) {
      fx::dim(140);
      fx::panel(Rect(24, 36, 80, 58));
      fx::big_text("PAUSED", Point(64, 47), 2, chili::theme.text);
      pause_menu.render(Rect(30, 60, 68, 30));
    }
  }
} play;

struct Entry : chili::Scene {
  chili::NameEntry name;
  void enter() override {
    name.start(save.data.last_name);
    audio::stop();
  }
  void update(uint32_t) override {
    if(!name.update(scenes.pressed())) return;
    ScoreEntry e{};
    memcpy(e.name, name.name(), 4);
    e.score = score;
    save.data.scores.insert(new_rank, e);
    memcpy(save.data.last_name, name.name(), 4);
    save.store();
    go_over();
  }
  void render(uint32_t time) override {
    draw_sky(time);
    fx::panel(Rect(10, 22, 108, 86));
    fx::big_text(new_rank == 0 ? "NEW BEST!" : "HIGH SCORE!", Point(64, 33), new_rank == 0 ? 2 : 1, chili::theme.gold);
    fx::text("ENTER YOUR NAME", Point(64, 50), chili::theme.dim, TextAlign::top_center);
    name.render(Point(64, 74), time);
    fx::text("A: OK   B: BACK", Point(64, 96), chili::theme.dim, TextAlign::top_center);
  }
} entry;

struct Over : chili::Scene {
  chili::ScoreTable table;
  void enter() override { audio::play(over_song); }
  void update(uint32_t) override {
    uint32_t pressed = scenes.pressed();
    if(pressed & Button::A) go_play();
    else if(pressed & (Button::B | Button::MENU)) go_title(150);
  }
  void render(uint32_t time) override {
    draw_sky(time);
    fx::panel(Rect(6, 8, 116, 112));
    fx::big_text("GAME OVER", Point(64, 19), 2, chili::theme.border);
    fx::text("SCORE " + std::to_string(score), Point(64, 32), chili::theme.text, TextAlign::top_center);
    table.render(save.data.scores, Rect(12, 46, 104, 45), new_rank, time);
    if(scenes.time_in_scene() >= scenes.guard_ms) {
      fx::text("A: AGAIN    B: MENU", Point(64, 104), chili::theme.text, TextAlign::top_center);
    }
  }
} over;

void go_title(uint32_t fade_ms) { scenes.go(title, fade_ms); }
void go_play() { scenes.go(play, 150); }
void go_over() { scenes.go(over); }
void go_results() {
  if(new_rank >= 0) scenes.go(entry, 300);
  else scenes.go(over, 300);
}

// ---------------------------------------------------------------------------

void init() {
  set_screen_mode(ScreenMode::hires);
  save.load();
  save.store_on_quit();
  if(save.data.speed > 2) save.data.speed = 1;

  chili::ui_sound = ui_sound;
  audio::init();
  apply_settings();

  scenes.guard_ms = 300;
  scenes.go(title);
}

void update(uint32_t time) {
  scenes.update(time);
  audio::update(time);
  uint32_t dt = scenes.dt();
  particles.update(dt);
  popups.update(dt);
  shake.update(dt);
}

void render(uint32_t time) {
  screen.alpha = 255;
  scenes.render(time);
}

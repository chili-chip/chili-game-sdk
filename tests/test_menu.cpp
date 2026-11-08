#include "check.hpp"
#include "fake_api.hpp"

#include "chili/menu.hpp"

#include <string>
#include <vector>

using namespace blit;

static std::vector<chili::UiSound> sounds;
static void record_sound(chili::UiSound s) { sounds.push_back(s); }

static void start() {
  sounds.clear();
  chili::ui_sound = record_sound;
}

TEST(up_down_moves_with_wrap_and_plays_move) {
  start();
  int played = 0;
  chili::Menu menu{{
    chili::Menu::action("PLAY", [&] { played++; }),
    chili::Menu::action("HELP", nullptr),
    chili::Menu::action("CREDITS", nullptr),
  }};
  CHECK_EQ(menu.selected(), 0);
  CHECK(menu.update(Button::DPAD_UP));
  CHECK_EQ(menu.selected(), 2);
  CHECK(menu.update(Button::DPAD_DOWN));
  CHECK_EQ(menu.selected(), 0);
  CHECK_EQ(sounds.size(), size_t(2));
  CHECK(sounds[0] == chili::UiSound::move);
  CHECK_EQ(played, 0);
}

TEST(a_runs_the_action_and_plays_select) {
  start();
  int played = 0;
  chili::Menu menu{{chili::Menu::action("PLAY", [&] { played++; })}};
  CHECK(menu.update(Button::A));
  CHECK_EQ(played, 1);
  CHECK_EQ(sounds.size(), size_t(1));
  CHECK(sounds[0] == chili::UiSound::select);
}

TEST(left_right_do_nothing_on_an_action) {
  start();
  int played = 0;
  chili::Menu menu{{chili::Menu::action("PLAY", [&] { played++; })}};
  CHECK(!menu.update(Button::DPAD_LEFT));
  CHECK(!menu.update(Button::DPAD_RIGHT));
  CHECK_EQ(played, 0);
  CHECK(sounds.empty());
}

TEST(toggle_flips_a_bool_with_a_left_and_right) {
  start();
  bool music = true;
  int changes = 0;
  chili::Menu menu{{chili::Menu::toggle("MUSIC", music, [&] { changes++; })}};
  menu.update(Button::A);
  CHECK(!music);
  menu.update(Button::DPAD_LEFT);
  CHECK(music);
  menu.update(Button::DPAD_RIGHT);
  CHECK(!music);
  CHECK_EQ(changes, 3);
  CHECK(sounds.back() == chili::UiSound::change);
}

TEST(toggle_works_on_a_byte_from_a_save) {
  start();
  uint8_t walls = 0;
  chili::Menu menu{{chili::Menu::toggle("WALLS", walls)}};
  menu.update(Button::A);
  CHECK_EQ(walls, 1);
  menu.update(Button::A);
  CHECK_EQ(walls, 0);
  CHECK(chili::Menu::value_text(menu.item(0)) == std::string("OFF"));
}

TEST(choice_cycles_with_wrap) {
  start();
  uint8_t speed = 1;
  chili::Menu menu{{chili::Menu::choice("SPEED", {"SLOW", "NORMAL", "FAST"}, speed)}};
  menu.update(Button::DPAD_RIGHT);
  CHECK_EQ(speed, 2);
  menu.update(Button::DPAD_RIGHT);
  CHECK_EQ(speed, 0);
  menu.update(Button::DPAD_LEFT);
  CHECK_EQ(speed, 2);
  menu.update(Button::A);
  CHECK_EQ(speed, 0);
  CHECK(chili::Menu::value_text(menu.item(0)) == std::string("SLOW"));
}

TEST(choice_recovers_from_an_index_out_of_range) {
  uint8_t speed = 7;  // say, from a corrupt save
  chili::Menu menu{{chili::Menu::choice("SPEED", {"SLOW", "NORMAL", "FAST"}, speed)}};
  CHECK(chili::Menu::value_text(menu.item(0)) == std::string(""));
  menu.update(Button::DPAD_RIGHT);
  CHECK(speed < 3);
}

TEST(b_calls_on_back) {
  start();
  int backs = 0;
  chili::Menu menu{{chili::Menu::action("RESUME", nullptr)}};
  CHECK(!menu.update(Button::B));  // no on_back set: nothing happens
  menu.on_back = [&] { backs++; };
  CHECK(menu.update(Button::B));
  CHECK_EQ(backs, 1);
  CHECK(sounds.back() == chili::UiSound::back);
}

TEST(select_sets_the_cursor) {
  chili::Menu menu{{chili::Menu::action("A", nullptr), chili::Menu::action("B", nullptr)}};
  menu.select(1);
  CHECK_EQ(menu.selected(), 1);
  menu.select(5);
  CHECK_EQ(menu.selected(), 1);
}

TEST(update_reads_buttons_by_default) {
  chili::Menu menu{{chili::Menu::action("A", nullptr), chili::Menu::action("B", nullptr)}};
  fake::hold(Button::DPAD_DOWN);
  CHECK(menu.update());
  CHECK_EQ(menu.selected(), 1);
}

TEST(nothing_pressed_returns_false) {
  chili::Menu menu{{chili::Menu::action("A", nullptr)}};
  CHECK(!menu.update(0));
}

struct RecordingMenu : chili::Menu {
  using chili::Menu::Menu;
  std::vector<Rect> rows;
  int selected_row = -1;
  bool background = false;
  void draw_background(Rect) override { background = true; }
  void draw_item(const Item &, Rect row, bool selected) override {
    if(selected) selected_row = int(rows.size());
    rows.push_back(row);
  }
};

TEST(render_lays_rows_out_top_to_bottom) {
  RecordingMenu menu{{chili::Menu::action("A", nullptr), chili::Menu::action("B", nullptr)}};
  menu.row_height = 9;
  menu.select(1);
  menu.render(Rect(14, 42, 100, 64));
  CHECK(menu.background);
  CHECK_EQ(menu.rows.size(), size_t(2));
  CHECK(menu.rows[0] == Rect(14, 42, 100, 9));
  CHECK(menu.rows[1] == Rect(14, 51, 100, 9));
  CHECK_EQ(menu.selected_row, 1);
}

static bool sound_setting = false;
static bool sound_applied = false;
static std::vector<bool> heard;
static void record_if_on(chili::UiSound) { heard.push_back(sound_applied); }

TEST(change_applies_before_its_sound) {
  // Turning sound on should already be on when the blip plays.
  heard.clear();
  sound_setting = false;
  sound_applied = false;
  chili::ui_sound = record_if_on;
  chili::Menu menu{{chili::Menu::toggle("SOUND", sound_setting, [] { sound_applied = sound_setting; })}};
  menu.update(Button::A);
  CHECK_EQ(heard.size(), size_t(1));
  CHECK(heard[0]);
}

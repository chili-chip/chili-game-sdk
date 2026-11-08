#include "check.hpp"
#include "fake_api.hpp"

#include "chili/screens.hpp"

#include <cstring>
#include <string>
#include <vector>

using namespace blit;

static std::vector<chili::UiSound> sounds;
static void record_sound(chili::UiSound s) { sounds.push_back(s); }

TEST(name_entry_starts_from_the_last_name) {
  chili::NameEntry e;
  e.start("KAC");
  CHECK(strcmp(e.name(), "KAC") == 0);
  CHECK_EQ(e.cursor(), 0);
}

TEST(name_entry_cleans_a_bad_last_name) {
  chili::NameEntry e;
  e.start("a?");
  CHECK(strcmp(e.name(), "AAA") == 0);
}

TEST(up_and_down_change_the_letter_with_wrap) {
  sounds.clear();
  chili::ui_sound = record_sound;
  chili::NameEntry e;
  e.start("AZA");
  e.update(Button::DPAD_DOWN);
  CHECK_EQ(e.name()[0], 'Z');
  e.update(Button::DPAD_UP);
  CHECK_EQ(e.name()[0], 'A');
  e.update(Button::DPAD_UP);
  CHECK_EQ(e.name()[0], 'B');
  CHECK(sounds.back() == chili::UiSound::change);
}

TEST(left_and_right_move_and_stop_at_the_ends) {
  chili::NameEntry e;
  e.start("AAA");
  e.update(Button::DPAD_LEFT);
  CHECK_EQ(e.cursor(), 0);
  e.update(Button::DPAD_RIGHT);
  e.update(Button::DPAD_RIGHT);
  e.update(Button::DPAD_RIGHT);
  CHECK_EQ(e.cursor(), 2);
  e.update(Button::DPAD_UP);
  CHECK(strcmp(e.name(), "AAB") == 0);
}

TEST(b_steps_back) {
  sounds.clear();
  chili::ui_sound = record_sound;
  chili::NameEntry e;
  e.start("AAA");
  CHECK(!e.update(Button::B));  // already on the first letter
  CHECK(sounds.empty());
  e.update(Button::A);
  CHECK_EQ(e.cursor(), 1);
  e.update(Button::B);
  CHECK_EQ(e.cursor(), 0);
  CHECK(sounds.back() == chili::UiSound::back);
}

TEST(a_on_the_last_letter_finishes) {
  chili::NameEntry e;
  e.start("ABC");
  CHECK(!e.update(Button::A));
  CHECK(!e.update(Button::A));
  CHECK(e.update(Button::A));
  CHECK(strcmp(e.name(), "ABC") == 0);
}

TEST(text_screen_closes_on_a_b_or_menu) {
  chili::TextScreen help("HELP", {"Eat apples."});
  CHECK(!help.update(Button::DPAD_UP));
  CHECK(help.update(Button::A));
  CHECK(help.update(Button::B));
  CHECK(help.update(Button::MENU));
  CHECK_EQ(help.lines.size(), size_t(1));
}

struct Entry {
  char name[4];
  uint32_t score;
};

struct RecordingTable : chili::ScoreTable {
  struct Row {
    int place;
    std::string name;
    uint32_t score;
    Rect rect;
    bool highlight;
  };
  std::vector<Row> rows;
  void draw_row(int place, const char *name, uint32_t score, Rect rect, bool highlight, uint32_t) override {
    rows.push_back({place, name ? name : "-", score, rect, highlight});
  }
};

TEST(score_table_draws_every_slot) {
  chili::HighScores<Entry, 3> scores{};
  scores.add({"AAA", 300});
  scores.add({"BBB", 100});
  RecordingTable table;
  table.render(scores, Rect(14, 34, 100, 27), 1);
  CHECK_EQ(table.rows.size(), size_t(3));
  CHECK(table.rows[0].name == "AAA");
  CHECK(table.rows[1].highlight);
  CHECK(!table.rows[0].highlight);
  CHECK(table.rows[2].name == "-");  // empty slot
  CHECK(table.rows[2].rect == Rect(14, 52, 100, 9));
}

TEST(score_text_is_the_number_by_default) {
  chili::ScoreTable table;
  CHECK(table.score_text(1234) == "1234");
}

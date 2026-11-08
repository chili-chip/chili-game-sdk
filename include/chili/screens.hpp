#pragma once

#include "32blit.hpp"
#include "chili/scores.hpp"
#include "chili/ui.hpp"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

// Ready-made screens around a game: name entry for a high score, help and
// credits pages, and the high-score table. Each one draws with chili::theme
// and can be restyled by overriding its draw hooks.
namespace chili {

// Three-letter name entry, arcade style.
//
// Up/down changes the letter (A-Z, wraps). Left/right moves. A goes to the
// next letter and, on the last one, finishes. B goes back a letter.
class NameEntry {
public:
  virtual ~NameEntry() = default;

  // Starts over from `last` (usually the last name used). Anything that is
  // not A-Z becomes A.
  void start(const char *last = "AAA");

  // Handles one frame of input. Returns true when the name is done.
  bool update(uint32_t pressed = blit::buttons.pressed);

  // The three letters, null-terminated.
  const char *name() const { return letters; }
  int cursor() const { return pos; }

  // Draws the three letters 18 px apart, centred on `center`.
  void render(blit::Point center, uint32_t time);

protected:
  // The default: the letter at 2x, with bobbing arrows above and below the
  // selected one.
  virtual void draw_letter(char letter, blit::Point center, bool selected, uint32_t time);

private:
  char letters[4] = "AAA";
  int pos = 0;
};

// A help or credits page. A, B or MENU closes it.
class TextScreen {
public:
  TextScreen(const char *title, std::initializer_list<const char *> lines);
  virtual ~TextScreen() = default;

  // Returns true when the player closes the screen.
  bool update(uint32_t pressed = blit::buttons.pressed);

  // The default: the title at the top, the lines centred in a panel, and
  // "B: BACK" at the bottom.
  virtual void render(uint32_t time);

  const char *title;
  std::vector<const char *> lines;
  int line_height = 8;
};

// Draws a HighScores table, one row per slot.
//
// Entries need `char name[4]` and `uint32_t score`. Override score_text() to
// show times, or draw_row() to add columns.
class ScoreTable {
public:
  virtual ~ScoreTable() = default;

  // Draws the rows from the top of `area`, `row_height` apart. The row at
  // `highlight` (for example, a new score) blinks.
  template<typename Entry, int N, Order O>
  void render(const HighScores<Entry, N, O> &table, blit::Rect area, int highlight = -1, uint32_t time = 0) {
    for(int i = 0; i < N; i++) {
      const Entry &e = table[i];
      blit::Rect row(area.x, area.y + i * row_height, area.w, row_height);
      draw_row(i, e.score ? e.name : nullptr, e.score, row, i == highlight, time);
    }
  }

  // How a score reads. The default is the plain number.
  virtual std::string score_text(uint32_t score) const;

  int row_height = 9;

protected:
  // The default: "1.", the name, the score on the right; "---" for an empty
  // slot (name is nullptr); gold for first place.
  virtual void draw_row(int place, const char *name, uint32_t score, blit::Rect row, bool highlight, uint32_t time);
};

}

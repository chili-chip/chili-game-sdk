#include "chili/screens.hpp"

#include "chili/fx.hpp"

using namespace blit;

namespace chili {

// ---------------------------------------------------------------------------
// NameEntry

void NameEntry::start(const char *last) {
  bool ended = false;
  for(int i = 0; i < 3; i++) {
    char c = ended ? 0 : last[i];
    if(!c) ended = true;
    letters[i] = (c >= 'A' && c <= 'Z') ? c : 'A';
  }
  letters[3] = 0;
  pos = 0;
}

bool NameEntry::update(uint32_t pressed) {
  char &c = letters[pos];
  if(pressed & Button::DPAD_UP) {
    c = c >= 'Z' ? 'A' : c + 1;
    play_ui_sound(UiSound::change);
  }
  if(pressed & Button::DPAD_DOWN) {
    c = c <= 'A' ? 'Z' : c - 1;
    play_ui_sound(UiSound::change);
  }
  if((pressed & Button::DPAD_LEFT) && pos > 0) {
    pos--;
    play_ui_sound(UiSound::move);
  }
  if((pressed & Button::DPAD_RIGHT) && pos < 2) {
    pos++;
    play_ui_sound(UiSound::move);
  }
  if((pressed & Button::B) && pos > 0) {
    pos--;
    play_ui_sound(UiSound::back);
  }
  if(pressed & Button::A) {
    play_ui_sound(UiSound::select);
    if(pos < 2) {
      pos++;
      return false;
    }
    return true;
  }
  return false;
}

void NameEntry::render(Point center, uint32_t time) {
  for(int i = 0; i < 3; i++) {
    draw_letter(letters[i], Point(center.x + (i - 1) * 18, center.y), i == pos, time);
  }
}

void NameEntry::draw_letter(char letter, Point c, bool selected, uint32_t time) {
  char s[2] = {letter, 0};
  fx::big_text(s, c, 2, selected ? theme.accent : theme.text);
  if(!selected) return;
  int bob = (time / 200) & 1;
  screen.pen = theme.accent;
  screen.triangle(Point(c.x - 3, c.y - 9 - bob), Point(c.x + 3, c.y - 9 - bob), Point(c.x, c.y - 12 - bob));
  screen.triangle(Point(c.x - 3, c.y + 9 + bob), Point(c.x + 3, c.y + 9 + bob), Point(c.x, c.y + 12 + bob));
}

// ---------------------------------------------------------------------------
// TextScreen

TextScreen::TextScreen(const char *title, std::initializer_list<const char *> lines) : title(title), lines(lines) {}

bool TextScreen::update(uint32_t pressed) {
  if(pressed & (Button::A | Button::B | Button::MENU)) {
    play_ui_sound(UiSound::back);
    return true;
  }
  return false;
}

void TextScreen::render(uint32_t time) {
  int w = screen.bounds.w;
  int h = screen.bounds.h;
  int cx = w / 2;
  fx::big_text(title, Point(cx, 12), 2, theme.accent);

  int body = int(lines.size()) * line_height;
  Rect box(6, 26, w - 12, body + 10);
  fx::panel(box);
  int y = box.y + 6;
  for(const char *l : lines) {
    fx::text(l, Point(cx, y), theme.text, TextAlign::top_center);
    y += line_height;
  }
  fx::text("B: BACK", Point(cx, h - 14), theme.dim, TextAlign::top_center);
}

// ---------------------------------------------------------------------------
// ScoreTable

std::string ScoreTable::score_text(uint32_t score) const {
  return std::to_string(score);
}

void ScoreTable::draw_row(int place, const char *name, uint32_t score, Rect row, bool highlight, uint32_t time) {
  if(highlight) {
    Pen hl = theme.gold;
    hl.a = ((time / 250) & 1) ? 120 : 70;
    screen.pen = hl;
    screen.rectangle(row);
  }
  int y = row.y + (row.h - 7) / 2;
  fx::text(std::to_string(place + 1) + ".", Point(row.x + 4, y), theme.dim);
  if(!name) {
    fx::text("---", Point(row.x + 18, y), theme.dim);
    return;
  }
  Pen col = place == 0 ? theme.gold : theme.text;
  fx::text(name, Point(row.x + 18, y), col);
  fx::text(score_text(score), Point(row.x + row.w - 4, y), col, TextAlign::top_right);
}

}

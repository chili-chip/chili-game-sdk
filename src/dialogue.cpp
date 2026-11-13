#include "chili/dialogue.hpp"

#include "chili/fx.hpp"
#include "chili/input.hpp"
#include "chili/ui.hpp"

#include <algorithm>

using namespace blit;

namespace chili {

namespace {
constexpr int TEXT_W = 104;   // next to the face
constexpr int FACE_X = 2;
constexpr int LINE_H = 8;
constexpr uint32_t MOUTH_MS = 110;
}

void TextBox::say(std::string_view text, std::string_view who, int face_sprite, const Surface *faces) {
  open_ = true;
  name = who;
  face = faces ? face_sprite : -1;
  sheet = faces;
  options.clear();
  picked = -1;
  cursor = 0;
  page_ = 0;

  int width = face >= 0 ? TEXT_W : 128 - 8;
  std::string wrapped = screen.wrap_text(std::string(text), width, minimal_font);
  lines.clear();
  size_t from = 0;
  while(from <= wrapped.size()) {
    size_t nl = wrapped.find('\n', from);
    if(nl == std::string::npos) nl = wrapped.size();
    lines.push_back(wrapped.substr(from, nl - from));
    from = nl + 1;
  }
  open_page();
}

void TextBox::ask(const std::vector<std::string> &choices) {
  options.assign(choices.begin(), choices.begin() + std::min<size_t>(choices.size(), 4));
}

void TextBox::close() {
  open_ = false;
  lines.clear();
  options.clear();
}

int TextBox::pages() const {
  return std::max<int>(1, (int(lines.size()) + LINES_PER_PAGE - 1) / LINES_PER_PAGE);
}

int TextBox::page_chars() const {
  int n = 0;
  for(int i = page_ * LINES_PER_PAGE; i < int(lines.size()) && i < (page_ + 1) * LINES_PER_PAGE; i++) n += lines[i].size();
  return n;
}

void TextBox::open_page() {
  revealed = 0;
  shown_ms = 0;
}

bool TextBox::update(uint32_t dt, uint32_t pressed) {
  if(!open_) return false;
  shown_ms += dt;

  if(typing()) {
    int due = std::min<int>(page_chars(), shown_ms / std::max<uint32_t>(char_ms, 1));
    if(due / 2 > revealed / 2 && on_letter) on_letter();
    revealed = due;
  }

  if(choosing()) {
    if(menu_nav(cursor, options.size(), pressed)) play_ui_sound(UiSound::move);
    if(pressed & Button::A) {
      play_ui_sound(UiSound::select);
      picked = cursor;
      close();
      return true;
    }
    return false;
  }

  if(shown_ms < input_delay_ms) return false;
  if(pressed & (Button::A | Button::B)) {
    if(typing()) {
      revealed = page_chars();  // the rest of the page at once
      return false;
    }
    if(!(pressed & Button::A)) return false;
    if(page_ + 1 < pages()) {
      play_ui_sound(UiSound::move);
      page_++;
      open_page();
      return false;
    }
    if(!options.empty()) return false;
    play_ui_sound(UiSound::select);
    close();
    return true;
  }
  return false;
}

void TextBox::draw_box(Rect box) {
  screen.pen = theme.panel;
  screen.rectangle(box);
  screen.pen = theme.border;
  screen.h_span(box.tl(), box.w);
}

void TextBox::render(uint32_t time) {
  if(!open_) return;
  int w = screen.bounds.w;
  int top = screen.bounds.h - BOX_H;
  draw_box(Rect(0, top, w, BOX_H));

  int text_x = 4;
  if(face >= 0 && sheet) {
    // the face talks (bobs a pixel) while letters appear
    bool mouth = typing() && (shown_ms / MOUTH_MS) % 2;
    Rect f(FACE_X, top + 4 - (mouth ? 1 : 0), 16, 16);
    screen.pen = theme.border;
    screen.rectangle(Rect(f.x - 1, f.y - 1, f.w + 2, f.h + 2));
    screen.pen = theme.panel;
    screen.rectangle(f);
    int cols = std::max<int>(1, sheet->bounds.w / 8);
    screen.stretch_blit(const_cast<Surface *>(sheet), Rect((face % cols) * 8, (face / cols) * 8, 8, 8), f);
    text_x = 22;
  }

  int y = top + 3;
  if(!name.empty()) fx::text(name, Point(text_x, y), theme.gold);
  y += LINE_H + 1;
  int left = revealed;
  for(int i = page_ * LINES_PER_PAGE; i < int(lines.size()) && i < (page_ + 1) * LINES_PER_PAGE && left > 0; i++, y += LINE_H) {
    int n = std::min<int>(left, lines[i].size());
    fx::text(std::string_view(lines[i]).substr(0, n), Point(text_x, y), theme.text);
    left -= n;
  }

  // a bobbing arrow at the top right when A goes on
  if(!typing() && shown_ms >= input_delay_ms && options.empty()) {
    Point a(w - 6, top + 4 + int((time / 250) % 2));
    screen.pen = theme.accent;
    screen.h_span(a + Point(-2, 0), 5);
    screen.h_span(a + Point(-1, 1), 3);
    screen.pixel(a + Point(0, 2));
  }

  if(choosing()) {
    int width = 0;
    for(auto &o : options) width = std::max<int>(width, screen.measure_text(o, minimal_font).w);
    Rect r(w - width - 16, top - 4 - int(options.size()) * 9 - 2, width + 14, int(options.size()) * 9 + 4);
    fx::panel(r);
    for(int i = 0; i < int(options.size()); i++) {
      Point p(r.x + 10, r.y + 3 + i * 9);
      if(i == cursor) fx::text(">", Point(r.x + 3, p.y), theme.accent);
      fx::text(options[i], p, i == cursor ? theme.text : theme.dim);
    }
  }
}

}

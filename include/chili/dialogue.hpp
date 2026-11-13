#pragma once

#include "32blit.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace chili {

// A text box along the bottom of the screen, for dialogue and story text.
//
// - Text appears letter by letter (typewriter), 3 lines per page.
// - A shows the whole page, then goes to the next one.
// - B shows the whole page (it does not skip ahead).
// - An optional speaker name and a 16x16 face (an 8x8 sprite drawn at 2x).
// - Up to 4 choices on the last page: up/down picks, A confirms.
//
// It looks and behaves like the Open Sky story box in Space Fighters TURBO,
// with the colours from chili::theme.
//
//   chili::TextBox box;
//   box.say("Slimes took the well.", "ELDER", 16, sheet);
//   box.ask({"Help", "Later"});
//   ...
//   if(box.update(dt)) answer = box.choice();
//   box.render(time);
class TextBox {
public:
  virtual ~TextBox() = default;

  // Opens the box with `text`, wrapped to fit. Use "\n" to start a new line.
  // An empty name leaves out the name row; face < 0 leaves out the face.
  void say(std::string_view text, std::string_view name = {}, int face = -1, const blit::Surface *sheet = nullptr);
  // Adds choices (up to 4) after the last page of the current text.
  void ask(const std::vector<std::string> &options);
  void close();

  // Handles one frame. Returns true on the frame the player finishes the
  // text: A after the last page, or A on a choice.
  bool update(uint32_t dt, uint32_t pressed = blit::buttons.pressed);
  // Draws the box (and the choices) when it is open.
  void render(uint32_t time);

  bool active() const { return open_; }
  // The choice picked, or -1.
  int choice() const { return picked; }
  // True while letters are still appearing.
  bool typing() const { return revealed < page_chars(); }
  int page() const { return page_; }
  int pages() const;

  uint32_t char_ms = 28;          // time per letter
  uint32_t input_delay_ms = 250;  // A is ignored this long, so mashing does not skip text
  // Called every other letter, for a talking blip.
  std::function<void()> on_letter;

  static constexpr int LINES_PER_PAGE = 3;
  static constexpr int BOX_H = 39;  // the box is the bottom 39 px of the screen

protected:
  // The default: a dark band with a line along the top (theme.panel, theme.border).
  virtual void draw_box(blit::Rect box);

private:
  int page_chars() const;
  void open_page();
  bool choosing() const { return !options.empty() && page_ == pages() - 1 && !typing() && shown_ms >= input_delay_ms; }

  bool open_ = false;
  std::string name;
  int face = -1;
  const blit::Surface *sheet = nullptr;
  std::vector<std::string> lines;  // wrapped
  std::vector<std::string> options;
  int cursor = 0;
  int picked = -1;
  int page_ = 0;
  int revealed = 0;
  uint32_t shown_ms = 0;
};

}

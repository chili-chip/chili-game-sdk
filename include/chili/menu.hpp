#pragma once

#include "32blit.hpp"
#include "chili/ui.hpp"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

namespace chili {

// A list of rows for title, options and pause menus.
//
// - Up/down moves the cursor, with wrap-around.
// - Left/right changes a toggle or a choice.
// - A runs an action, flips a toggle or steps a choice forward.
// - B calls on_back, if set.
//
// Toggles and choices write straight into your variables (usually fields of
// the save struct). Restyle a menu by overriding draw_item() and
// draw_background(); the defaults use chili::theme.
class Menu {
public:
  enum class Kind : uint8_t { action, toggle, choice };

  struct Item {
    Kind kind = Kind::action;
    const char *label = "";
    // Action: runs on A. Toggle and choice: runs after the value changed.
    std::function<void()> on_select;
    bool *flag = nullptr;      // toggle on a bool
    uint8_t *value = nullptr;  // toggle on a byte, or the choice index
    std::vector<const char *> options;
  };

  static Item action(const char *label, std::function<void()> on_select);
  static Item toggle(const char *label, bool &value, std::function<void()> on_change = nullptr);
  static Item toggle(const char *label, uint8_t &value, std::function<void()> on_change = nullptr);
  static Item choice(const char *label, std::initializer_list<const char *> options, uint8_t &index,
                     std::function<void()> on_change = nullptr);

  Menu(std::initializer_list<Item> items);
  virtual ~Menu() = default;

  // Handles one frame of input. Returns true when something happened.
  // `pressed` defaults to this frame's presses.
  bool update(uint32_t pressed = blit::buttons.pressed);

  // Draws the background, then each row, `row_height` apart from the top of
  // `area`.
  void render(blit::Rect area);

  int selected() const { return cursor; }
  // Moves the cursor; ignored when out of range.
  void select(int index);

  int count() const { return int(rows.size()); }
  const Item &item(int index) const { return rows[index]; }

  // "ON"/"OFF" for a toggle, the option for a choice, "" for an action.
  static std::string value_text(const Item &item);

  // Called when B is pressed, for example to close a pause menu.
  std::function<void()> on_back;
  int row_height = 10;

protected:
  virtual void draw_background(blit::Rect area);
  // The default: a highlight bar on the selected row, the label on the left,
  // the value on the right ("< NORMAL >" when selected).
  virtual void draw_item(const Item &item, blit::Rect row, bool selected);

private:
  bool change(int delta);

  std::vector<Item> rows;
  int cursor = 0;
};

}

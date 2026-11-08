#include "chili/menu.hpp"

#include "chili/fx.hpp"
#include "chili/input.hpp"

using namespace blit;

namespace chili {

Menu::Item Menu::action(const char *label, std::function<void()> on_select) {
  Item item;
  item.kind = Kind::action;
  item.label = label;
  item.on_select = std::move(on_select);
  return item;
}

Menu::Item Menu::toggle(const char *label, bool &value, std::function<void()> on_change) {
  Item item;
  item.kind = Kind::toggle;
  item.label = label;
  item.flag = &value;
  item.on_select = std::move(on_change);
  return item;
}

Menu::Item Menu::toggle(const char *label, uint8_t &value, std::function<void()> on_change) {
  Item item;
  item.kind = Kind::toggle;
  item.label = label;
  item.value = &value;
  item.on_select = std::move(on_change);
  return item;
}

Menu::Item Menu::choice(const char *label, std::initializer_list<const char *> options, uint8_t &index,
                        std::function<void()> on_change) {
  Item item;
  item.kind = Kind::choice;
  item.label = label;
  item.value = &index;
  item.options = options;
  item.on_select = std::move(on_change);
  return item;
}

Menu::Menu(std::initializer_list<Item> items) : rows(items) {}

void Menu::select(int index) {
  if(index >= 0 && index < count()) cursor = index;
}

bool Menu::change(int delta) {
  Item &item = rows[cursor];
  if(item.kind == Kind::toggle) {
    if(item.flag) *item.flag = !*item.flag;
    else *item.value = *item.value ? 0 : 1;
  } else if(item.kind == Kind::choice) {
    int n = int(item.options.size());
    if(n == 0) return false;
    *item.value = uint8_t(((*item.value % n) + n + delta) % n);
  } else {
    return false;
  }
  play_ui_sound(UiSound::change);
  if(item.on_select) item.on_select();
  return true;
}

bool Menu::update(uint32_t pressed) {
  if(rows.empty()) return false;

  if(menu_nav(cursor, count(), pressed)) {
    play_ui_sound(UiSound::move);
    return true;
  }
  if(pressed & Button::DPAD_LEFT) return change(-1);
  if(pressed & Button::DPAD_RIGHT) return change(1);

  if(pressed & Button::A) {
    Item &item = rows[cursor];
    if(item.kind != Kind::action) return change(1);
    play_ui_sound(UiSound::select);
    if(item.on_select) item.on_select();
    return true;
  }

  if((pressed & Button::B) && on_back) {
    play_ui_sound(UiSound::back);
    on_back();
    return true;
  }
  return false;
}

void Menu::render(Rect area) {
  draw_background(area);
  for(int i = 0; i < count(); i++) {
    draw_item(rows[i], Rect(area.x, area.y + i * row_height, area.w, row_height), i == cursor);
  }
}

std::string Menu::value_text(const Item &item) {
  switch(item.kind) {
    case Kind::toggle: {
      bool on = item.flag ? *item.flag : *item.value != 0;
      return on ? "ON" : "OFF";
    }
    case Kind::choice:
      return *item.value < item.options.size() ? item.options[*item.value] : "";
    case Kind::action:
      break;
  }
  return "";
}

void Menu::draw_background(Rect area) {}

void Menu::draw_item(const Item &item, Rect row, bool selected) {
  if(selected) {
    screen.pen = theme.highlight;
    screen.rectangle(row);
  }
  int y = row.y + (row.h - 7) / 2;
  fx::text(item.label, Point(row.x + 4, y), selected ? theme.text : theme.dim);

  std::string value = value_text(item);
  if(value.empty()) return;
  if(selected) value = "< " + value + " >";
  fx::text(value, Point(row.x + row.w - 4, y), selected ? theme.accent : theme.dim, TextAlign::top_right);
}

}

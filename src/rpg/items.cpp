// The bag, equipment, and the screens on top of the map: inventory, quest
// log and the HUD.

#include "chili/rpg/world.hpp"

#include "chili/input.hpp"
#include "chili/ui.hpp"

#include <algorithm>
#include <string>

using namespace blit;

namespace chili::rpg {

// ---------------------------------------------------------------------------
// The bag

int World::count(Index item) const {
  int n = 0;
  for(auto &s : state.bag)
    if(s.item == item) n += s.count;
  return n;
}

int World::bag_used() const {
  int n = 0;
  for(int i = 0; i < game.bag_size; i++)
    if(state.bag[i].item != NONE) n++;
  return n;
}

int World::give(Index item, int n) {
  if(item < 0 || item >= Index(game.items.size())) return n;
  const Item &it = game.items[item];
  // fill stacks first, then empty slots
  for(int i = 0; i < game.bag_size && n > 0; i++) {
    BagSlot &s = state.bag[i];
    if(s.item != item || s.count >= it.stack) continue;
    int add = std::min(n, it.stack - s.count);
    s.count += add;
    n -= add;
  }
  for(int i = 0; i < game.bag_size && n > 0; i++) {
    BagSlot &s = state.bag[i];
    if(s.item != NONE) continue;
    s.item = item;
    s.count = std::min<int>(n, it.stack);
    s.equipped = false;
    n -= s.count;
  }
  return n;
}

void World::remove_slot(int i) {
  for(int k = i; k + 1 < MAX_BAG; k++) state.bag[k] = state.bag[k + 1];
  state.bag[MAX_BAG - 1] = BagSlot();
}

int World::take(Index item, int n) {
  int taken = 0;
  // unequipped ones first
  for(int pass = 0; pass < 2; pass++)
    for(int i = game.bag_size - 1; i >= 0 && taken < n; i--) {
      BagSlot &s = state.bag[i];
      if(s.item != item || s.equipped != (pass == 1)) continue;
      int t = std::min<int>(n - taken, s.count);
      s.count -= t;
      taken += t;
      if(s.count == 0) remove_slot(i);
    }
  return taken;
}

void World::use_item(int i) {
  if(i < 0 || i >= game.bag_size || state.bag[i].item == NONE) return;
  Index item = state.bag[i].item;
  const Item &it = game.items[item];
  if(it.kind == ItemKind::equip) {
    equip(i);
    return;
  }
  if(hooks.on_use_item && hooks.on_use_item(*this, item)) return;
  if(it.kind != ItemKind::use || it.use.empty()) {
    play(Sound::bump);
    return;
  }
  if(!check(it.use) && !it.use.else_count) {
    play(Sound::bump);
    return;
  }
  // used up first, so the item's event can give it back
  bool used = check(it.use);
  if(used) {
    state.bag[i].count--;
    if(state.bag[i].count == 0) remove_slot(i);
  }
  run(it.use);
  check_quests();
}

void World::equip(int i) {
  if(i < 0 || i >= game.bag_size || state.bag[i].item == NONE) return;
  BagSlot &s = state.bag[i];
  const Item &it = game.items[s.item];
  if(it.kind != ItemKind::equip) return;
  if(s.equipped) {
    s.equipped = false;
  } else {
    // one item per slot
    for(auto &o : state.bag)
      if(o.equipped && o.item != NONE && game.items[o.item].slot == it.slot) o.equipped = false;
    s.equipped = true;
  }
  play(Sound::door);
  // max hp can drop when gear comes off
  player.stats[game.hp] = std::min<int>(player.stats[game.hp], stat(player, game.max_hp));
}

void World::drop(int i) {
  if(i < 0 || i >= game.bag_size || state.bag[i].item == NONE) return;
  ItemKind k = game.items[state.bag[i].item].kind;
  if(k == ItemKind::key || k == ItemKind::quest) {
    play(Sound::bump);
    return;
  }
  if(!put(state.bag[i].item, state.bag[i].count, player.x, player.y)) {
    popup(player.x, player.y, "NO ROOM", theme.dim);
    play(Sound::bump);
    return;
  }
  bool was_equipped = state.bag[i].equipped;
  remove_slot(i);
  if(was_equipped) player.stats[game.hp] = std::min<int>(player.stats[game.hp], stat(player, game.max_hp));
}

// ---------------------------------------------------------------------------
// The floor

Index World::item_at(int x, int y) const {
  const Room &r = room();
  const RoomState &rs = state.rooms[state.room];
  for(size_t i = 0; i < r.items.size(); i++)
    if(!(rs.taken & (1u << i)) && r.items[i].x == x && r.items[i].y == y) return r.items[i].item;
  for(auto &f : state.floor)
    if(f.room == state.room && f.x == x && f.y == y) return f.item;
  return NONE;
}

bool World::put(Index item, int count, int x, int y) {
  if(item < 0 || item >= Index(game.items.size()) || count <= 0) return false;
  for(auto &f : state.floor)
    if(f.room == NONE) {
      f = FloorItem{state.room, item, uint8_t(x), uint8_t(y), uint8_t(std::min(count, 255))};
      return true;
    }
  return false;
}

bool World::pick_up() {
  const Room &r = room();
  RoomState &rs = state.rooms[state.room];
  bool found = false, full = false;
  auto take_one = [&](Index item, uint8_t &n) {
    found = true;
    int left = give(item, n);
    if(left < n) popup(player.x, player.y, game.items[item].name, theme.gold);
    if(left) full = true;
    n = uint8_t(left);
  };
  for(size_t i = 0; i < r.items.size(); i++) {
    const RoomItem &it = r.items[i];
    if(rs.taken & (1u << i) || it.x != player.x || it.y != player.y) continue;
    uint8_t n = it.count;
    take_one(it.item, n);
    if(n == 0) rs.taken |= 1u << i;
    else if(n < it.count) {  // part of it fit: the rest lies on the floor
      rs.taken |= 1u << i;
      put(it.item, n, it.x, it.y);
    }
  }
  for(auto &f : state.floor) {
    if(f.room != state.room || f.x != player.x || f.y != player.y) continue;
    take_one(f.item, f.count);
    if(f.count == 0) f = FloorItem{};
  }
  if(full) popup(player.x, player.y - 1, "BAG FULL", theme.dim);
  if(found && !full) play(Sound::pickup);
  if(found) check_quests();  // a collect goal can be met now
  return found;
}

// ---------------------------------------------------------------------------
// Screens

void World::open(Screen s) {
  screen_ = s;
  cursor = 0;
  drop_armed = false;
  play_ui_sound(s == Screen::none ? UiSound::back : UiSound::select);
}

void World::update_screen(uint32_t pressed) {
  if(screen_ == Screen::quests) {
    if(pressed & (Button::B | Button::Y)) open(Screen::none);
    return;
  }

  int used = bag_used();
  if(pressed & (Button::B | Button::X)) {
    if(drop_armed && (pressed & Button::X)) {
      drop(cursor);
      play_ui_sound(UiSound::back);
      drop_armed = false;
      cursor = std::min(cursor, std::max(0, bag_used() - 1));
      return;
    }
    if(pressed & Button::X && used) {
      drop_armed = true;  // X again drops
      return;
    }
    open(Screen::none);
    return;
  }
  if(used && menu_nav(cursor, used, pressed)) {
    drop_armed = false;
    play_ui_sound(UiSound::move);
  }
  if(used && (pressed & Button::A)) {
    drop_armed = false;
    use_item(cursor);
    cursor = std::min(cursor, std::max(0, bag_used() - 1));
  }
}

void World::draw_hud() {
  auto &scr = blit::screen;
  int hp = player.stats[game.hp], max = std::max(1, stat(player, game.max_hp));
  Rect bar(2, 2, 40, 5);
  scr.pen = theme.shadow;
  scr.rectangle(Rect(bar.x - 1, bar.y - 1, bar.w + 2, bar.h + 2));
  scr.pen = Pen(70, 20, 24);
  scr.rectangle(bar);
  scr.pen = hp * 4 <= max ? Pen(255, 90, 60) : Pen(220, 50, 60);
  scr.rectangle(Rect(bar.x, bar.y, bar.w * hp / max, bar.h));
  fx::text(std::to_string(hp) + "/" + std::to_string(max), Point(45, 1), theme.text);
  fx::text("LV" + std::to_string(state.level), Point(scr.bounds.w - 2, 1), theme.gold, TextAlign::top_right);
}

void World::draw_inventory(uint32_t time) {
  auto &scr = blit::screen;
  Rect r(4, 10, scr.bounds.w - 8, scr.bounds.h - 20);
  fx::panel(r);
  fx::text("BAG", Point(r.x + 4, r.y + 3), theme.accent);
  fx::text(std::to_string(bag_used()) + "/" + std::to_string(game.bag_size), Point(r.x + r.w - 4, r.y + 3), theme.dim,
           TextAlign::top_right);

  int used = bag_used();
  const int row_h = 10, rows = 5;
  int first = std::clamp(cursor - rows + 1, 0, std::max(0, used - rows));
  for(int i = first; i < used && i < first + rows; i++) {
    const BagSlot &s = state.bag[i];
    const Item &it = game.items[s.item];
    int y = r.y + 13 + (i - first) * row_h;
    if(i == cursor) {
      scr.pen = theme.highlight;
      scr.rectangle(Rect(r.x + 2, y - 1, r.w - 4, row_h));
    }
    draw_sprite(it.icon, Point(r.x + 4, y));
    std::string name = it.name;
    if(s.equipped) name += " (E)";
    fx::text(name, Point(r.x + 15, y + 1), i == cursor ? theme.text : theme.dim);
    if(s.count > 1) fx::text("x" + std::to_string(s.count), Point(r.x + r.w - 4, y + 1), theme.dim, TextAlign::top_right);
  }
  if(!used) fx::text("EMPTY", Point(scr.bounds.w / 2, r.y + 30), theme.dim, TextAlign::top_center);

  // the selected item's text, and what A does
  int y = r.y + r.h - 29;  // two lines of text, then the button hints
  scr.pen = theme.border;
  scr.h_span(Point(r.x + 2, y - 2), r.w - 4);
  if(used) {
    const BagSlot &s = state.bag[cursor];
    const Item &it = game.items[s.item];
    std::string text = it.text;
    if(it.kind == ItemKind::equip) {
      for(int k = 0; k < MAX_STATS && k < int(game.stats.size()); k++)
        if(it.bonus[k]) text += (text.empty() ? "" : " ") + std::string(it.bonus[k] > 0 ? "+" : "") + std::to_string(it.bonus[k]) + " " + game.stats[k].label;
    }
    fx::text(scr.wrap_text(text, r.w - 8, minimal_font), Point(r.x + 4, y), theme.text, TextAlign::top_left, false);
    const char *a = it.kind == ItemKind::equip ? (s.equipped ? "A: TAKE OFF" : "A: EQUIP") : it.kind == ItemKind::use ? "A: USE" : "";
    fx::text(drop_armed ? "X AGAIN: DROP" : a, Point(r.x + 4, r.y + r.h - 9), drop_armed ? theme.accent : theme.dim);
  }
  fx::text("B: BACK", Point(r.x + r.w - 4, r.y + r.h - 9), theme.dim, TextAlign::top_right);
}

void World::draw_quests(uint32_t time) {
  auto &scr = blit::screen;
  Rect r(4, 10, scr.bounds.w - 8, scr.bounds.h - 20);
  fx::panel(r);
  fx::text("QUESTS", Point(r.x + 4, r.y + 3), theme.accent);
  int y = r.y + 14;
  bool any = false;
  // active quests first, with what to do next, then the finished ones
  for(int pass = 0; pass < 2; pass++)
    for(size_t q = 0; q < game.quests.size(); q++) {
      uint8_t st = state.quests[q];
      bool done = st == State::DONE;
      if(st == 0 || done != (pass == 1)) continue;
      any = true;
      if(y > r.y + r.h - 18) break;
      const Quest &quest = game.quests[q];
      fx::text(done ? "+ " + quest.title : quest.title, Point(r.x + 4, y), done ? theme.dim : theme.gold);
      y += 9;
      if(!done && st <= quest.stages.size()) {
        const Stage &s = quest.stages[st - 1];
        std::string log = s.log;
        if(s.goal.type == Goal::Type::defeat || (s.goal.type == Goal::Type::collect && s.goal.count > 1)) {
          int have = s.goal.type == Goal::Type::defeat ? state.progress[q] : count(s.goal.target);
          log += " (" + std::to_string(std::min<int>(have, s.goal.count)) + "/" + std::to_string(s.goal.count) + ")";
        }
        std::string wrapped = scr.wrap_text(log, r.w - 12, minimal_font);
        fx::text(wrapped, Point(r.x + 8, y), theme.text, TextAlign::top_left, false);
        y += 8 * (1 + int(std::count(wrapped.begin(), wrapped.end(), '\n'))) + 3;
      }
    }
  if(!any) fx::text("NO QUESTS YET", Point(scr.bounds.w / 2, r.y + 30), theme.dim, TextAlign::top_center);
  fx::text("B: BACK", Point(r.x + r.w - 4, r.y + r.h - 9), theme.dim, TextAlign::top_right);
}

}

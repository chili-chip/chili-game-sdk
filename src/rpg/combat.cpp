// Bump combat, stats, xp and levels.

#include "chili/rpg/world.hpp"

#include "chili/ui.hpp"

#include <algorithm>
#include <string>

using namespace blit;

namespace chili::rpg {

namespace {
constexpr int TILE = 8;
}

int World::stat(const Actor &a, Index s) const {
  if(s < 0 || s >= MAX_STATS) return 0;
  int v = a.stats[s];
  if(a.is_player() && s != game.hp) {
    for(auto &slot : state.bag)
      if(slot.equipped && slot.item != NONE) v += game.items[slot.item].bonus[s];
  }
  return v;
}

bool World::next_to(const Actor &a, const Actor &b) const {
  int dx = std::abs(a.x - b.x), dy = std::abs(a.y - b.y);
  return game.diagonal ? std::max(dx, dy) == 1 : dx + dy == 1;
}

int World::damage(const Actor &attacker, const Actor &defender) {
  if(hooks.damage) return std::max(0, hooks.damage(*this, attacker, defender));
  int d = std::max(1, stat(attacker, game.atk) - stat(defender, game.def));
  if(game.spread) d = std::max(1, d + int(blit::random() % (game.spread * 2 + 1)) - game.spread);
  if(game.crit && int(blit::random() % 100) < game.crit) d *= 2;
  return d;
}

void World::attack(Actor &attacker, Actor &defender) {
  int dx = defender.x - attacker.x, dy = defender.y - attacker.y;
  if(dx < 0) attacker.facing = Dir::left;
  else if(dx > 0) attacker.facing = Dir::right;
  else if(dy < 0) attacker.facing = Dir::up;
  else if(dy > 0) attacker.facing = Dir::down;
  // a short lunge towards the target
  attacker.ox = int8_t(dx * 3);
  attacker.oy = int8_t(dy * 3);
  attacker.slide_ms = game.step_ms;

  int d = damage(attacker, defender);
  if(d <= 0) {
    play(Sound::miss);
    popup(defender.x, defender.y, "MISS", theme.dim);
    return;
  }
  hurt(defender, d);
}

void World::hurt(Actor &a, int amount) {
  if(!a.alive || amount <= 0) return;
  int16_t &hp = a.stats[game.hp];
  hp = std::max(0, hp - amount);
  a.flash_ms = 300;
  popup(a.x, a.y, std::to_string(amount), a.is_player() ? Pen(255, 90, 80) : Pen(255, 255, 255));
  particles.burst(Point(a.x * TILE + TILE / 2, a.y * TILE + TILE / 2), a.is_player() ? Pen(220, 50, 50) : Pen(240, 240, 240), 6);
  if(a.is_player()) {
    shake.start(200, 100);
    play(Sound::hurt);
  } else {
    play(Sound::hit);
  }
  if(hp == 0) die(a);
}

void World::heal(int amount) {
  int16_t &hp = player.stats[game.hp];
  int before = hp;
  hp = std::min(stat(player, game.max_hp), hp + amount);
  if(hp > before) popup(player.x, player.y, "+" + std::to_string(hp - before), Pen(100, 230, 120));
}

void World::die(Actor &a) {
  if(a.is_player()) {
    if(!game.lose.empty()) run(game.lose);
    // the lose event can save the player (heal and warp home); otherwise it is over
    if(player.stats[game.hp] <= 0 && result_ == Result::playing) end(false);
    return;
  }
  a.alive = false;
  if(a.spawn >= 0) state.rooms[state.room].gone |= 1u << a.spawn;
  particles.burst(Point(a.x * TILE + TILE / 2, a.y * TILE + TILE / 2), Pen(255, 200, 80), 16);

  const Kind &k = game.kinds[a.kind];
  for(size_t q = 0; q < game.quests.size(); q++) {
    uint8_t st = state.quests[q];
    if(st == 0 || st == State::DONE || st > game.quests[q].stages.size()) continue;
    const Goal &g = game.quests[q].stages[st - 1].goal;
    if(g.type == Goal::Type::defeat && g.target == a.kind) state.progress[q]++;
  }
  for(auto &d : k.drops)
    if(int(blit::random() % 100) < d.chance) put(d.item, d.count, a.x, a.y);
  run(k.death, &a);
  gain_xp(k.xp);
}

void World::gain_xp(int amount) {
  if(amount <= 0) return;
  state.xp += amount;
  while(state.level - 1 < int(game.xp.size()) && state.xp >= game.xp[state.level - 1]) level_up();
}

void World::level_up() {
  state.level++;
  for(int i = 0; i < MAX_STATS; i++) player.stats[i] += game.gain[i];
  // the new max hp comes with full health
  player.stats[game.hp] = stat(player, game.max_hp);
  play(Sound::level);
  popup(player.x, player.y, "LEVEL UP", theme.gold);
  if(hooks.on_level_up) hooks.on_level_up(*this);
}

}

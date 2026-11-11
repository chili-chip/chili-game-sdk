// Enemy brains. Each actor that is not the player thinks once per turn
// (every `speed` turns), picks a brain from its kind, and steps or attacks.

#include "chili/rpg/world.hpp"

#include <algorithm>
#include <cstdlib>

namespace chili::rpg {

namespace {

constexpr int MAX_SEARCH = 600;  // cells a path search looks at

int dist(int ax, int ay, int bx, int by) {
  return std::max(std::abs(ax - bx), std::abs(ay - by));
}

}

bool World::sees(const Actor &a, const Actor &b) const {
  if(dist(a.x, a.y, b.x, b.y) > game.kinds[a.kind].sight) return false;
  // walk a line from a to b; solid tiles block it
  int x = a.x, y = a.y;
  int dx = std::abs(b.x - x), dy = -std::abs(b.y - y);
  int sx = x < b.x ? 1 : -1, sy = y < b.y ? 1 : -1;
  int err = dx + dy;
  while(x != b.x || y != b.y) {
    int e2 = 2 * err;
    if(e2 >= dy) {
      err += dy;
      x += sx;
    }
    if(e2 <= dx) {
      err += dx;
      y += sy;
    }
    if((x != b.x || y != b.y) && blocked(x, y, true)) return false;
  }
  return true;
}

// The first step of a shortest path from `a` to (tx, ty), going round walls
// and other actors. Breadth-first search, capped at MAX_SEARCH cells.
bool World::path_step(const Actor &a, int tx, int ty, int &dx, int &dy) const {
  static int16_t from[MAX_ROOM * MAX_ROOM];
  static int16_t queue[MAX_SEARCH];
  const Room &r = room();
  int w = r.w;
  int start = a.y * w + a.x, goal = ty * w + tx;
  std::fill(from, from + r.w * r.h, int16_t(-1));
  from[start] = int16_t(start);
  int head = 0, tail = 0;
  queue[tail++] = int16_t(start);

  const int dirs = game.diagonal ? 8 : 4;
  const int DX[8] = {1, -1, 0, 0, 1, 1, -1, -1};
  const int DY[8] = {0, 0, 1, -1, 1, -1, 1, -1};
  bool found = false;
  while(head < tail && !found) {
    int c = queue[head++];
    int cx = c % w, cy = c / w;
    for(int d = 0; d < dirs; d++) {
      int nx = cx + DX[d], ny = cy + DY[d];
      if(nx < 0 || ny < 0 || nx >= r.w || ny >= r.h) continue;
      int n = ny * w + nx;
      if(from[n] >= 0) continue;
      if(n != goal) {
        if(blocked(nx, ny)) continue;
        if(d >= 4 && (blocked(cx + DX[d], cy) || blocked(cx, cy + DY[d]))) continue;
        bool taken = false;
        for(auto &o : actors)
          if(o.alive && o.x == nx && o.y == ny) taken = true;
        if(taken || (player.x == nx && player.y == ny)) continue;
      }
      from[n] = int16_t(c);
      if(n == goal) {
        found = true;
        break;
      }
      if(tail < MAX_SEARCH) queue[tail++] = int16_t(n);
    }
  }
  if(!found) return false;
  int c = goal;
  while(from[c] != start) c = from[c];
  dx = c % w - a.x;
  dy = c / w - a.y;
  return true;
}

void World::wander(Actor &a) {
  int d = blit::random() % 6;  // 4 directions, or stay
  const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
  if(d < 4) step(a, DX[d], DY[d]);
}

void World::step_away(Actor &a, const Actor &from) {
  int best = dist(a.x, a.y, from.x, from.y), bx = 0, by = 0;
  const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
  for(int d = 0; d < 4; d++) {
    int nx = a.x + DX[d], ny = a.y + DY[d];
    if(blocked(nx, ny) || actor_at(nx, ny)) continue;
    int far = dist(nx, ny, from.x, from.y);
    if(far > best) {
      best = far;
      bx = DX[d];
      by = DY[d];
    }
  }
  if(bx || by) step(a, bx, by);
}

void World::think(Actor &a) {
  const Kind &k = game.kinds[a.kind];
  if(k.brain == Brain::custom) {
    auto it = hooks.brains.find(game.brains[k.custom]);
    if(it != hooks.brains.end() && it->second) it->second(*this, a);
    return;
  }

  bool hostile = !k.friendly && player.alive;
  bool scared = k.brain == Brain::coward && a.stats[game.hp] * 100 < stat(a, game.max_hp) * k.flee;
  if(hostile && !scared && next_to(a, player)) {
    attack(a, player);
    return;
  }

  int dx = 0, dy = 0;
  bool sees_player = hostile && sees(a, player);
  switch(k.brain) {
    case Brain::still:
    case Brain::custom: break;
    case Brain::wander: wander(a); break;
    case Brain::chase:
      if(sees_player && path_step(a, player.x, player.y, dx, dy)) step(a, dx, dy);
      break;
    case Brain::guard:
      // chase while near home, else walk back
      if(sees_player && dist(player.x, player.y, a.home_x, a.home_y) <= k.home) {
        if(path_step(a, player.x, player.y, dx, dy)) step(a, dx, dy);
      } else if((a.x != a.home_x || a.y != a.home_y) && path_step(a, a.home_x, a.home_y, dx, dy)) {
        step(a, dx, dy);
      }
      break;
    case Brain::patrol: {
      if(a.spawn < 0) break;
      const auto &points = room().actors[a.spawn].patrol;
      if(points.empty()) break;
      const Spot &p = points[a.patrol_at % points.size()];
      if(a.x == p.x && a.y == p.y) {
        a.patrol_at = (a.patrol_at + 1) % points.size();
        break;
      }
      if(path_step(a, p.x, p.y, dx, dy)) step(a, dx, dy);
      break;
    }
    case Brain::coward:
      if(scared && sees_player) step_away(a, player);
      else if(sees_player && path_step(a, player.x, player.y, dx, dy)) step(a, dx, dy);
      break;
  }
}

}

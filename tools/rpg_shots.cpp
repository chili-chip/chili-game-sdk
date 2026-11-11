// Plays Pepper Village (the RPG demo) with scripted input, on the fake 32blit
// API from the tests, and writes screenshots as PPM files into the folder
// given as the first argument. tools/screenshots.py turns them into PNGs.

#include "../examples/rpg/rpg.cpp"

#include "fake_api.hpp"

#include <cstdio>

static uint32_t clock_ms = 0;
static const char *out_dir = ".";

static void run(uint32_t ms, uint32_t held = 0) {
  for(uint32_t t = 0; t < ms; t += 10) {
    fake::hold(held);
    clock_ms += 10;
    ::update(clock_ms);
  }
}

static void tap(uint32_t button, uint32_t wait_ms = 200) {
  run(10, button);
  run(wait_ms);
}

static void shot(const char *name) {
  ::render(clock_ms);
  char path[512];
  snprintf(path, sizeof(path), "%s/%s.ppm", out_dir, name);
  FILE *f = fopen(path, "wb");
  if(!f) return;
  fprintf(f, "P6\n128 128\n255\n");
  fwrite(fake::screen_pixels(), 1, 128 * 128 * 3, f);
  fclose(f);
  printf("%s\n", path);
}

static void place(int x, int y) {
  world.player.x = x;
  world.player.y = y;
}

// Presses A until the dialogue is over.
static void read_all() {
  for(int i = 0; i < 20 && world.talking(); i++) tap(Button::A, 1500);
}

static rpg::Actor *find(const char *kind) {
  rpg::Index k = rpg::Game::find(world.game.kinds, kind);
  for(auto &a : world.actors)
    if(a.alive && a.kind == k) return &a;
  return nullptr;
}

int main(int argc, char **argv) {
  if(argc > 1) out_dir = argv[1];
  fake::reset();
  ::init();

  run(800);
  shot("rpg-title");

  tap(Button::A, 900);  // NEW GAME
  world.show_hud = false;
  shot("rpg-splash");
  world.show_hud = true;
  shot("rpg-village");

  // talk to the elder
  place(5, 3);
  tap(Button::DPAD_LEFT, 2500);
  shot("rpg-dialogue");
  tap(Button::A, 2500);
  shot("rpg-choice");
  tap(Button::A, 2500);  // Yes
  read_all();

  // the smith gives a sword
  place(10, 3);
  tap(Button::DPAD_LEFT, 3000);
  read_all();
  run(300);

  tap(Button::X, 300);
  tap(Button::A, 300);  // equip the sword
  shot("rpg-inventory");
  tap(Button::B, 300);

  tap(Button::Y, 300);
  shot("rpg-quests");
  tap(Button::B, 300);

  // a fight in the cave
  world.warp(rpg::Game::find(world.game.rooms, "cave"), 2, 2, false);
  world.state.turn = 57;
  if(rpg::Actor *s = find("spider")) {
    place(s->x - 1, s->y);
    tap(Button::DPAD_RIGHT, 0);
    run(120);
    shot("rpg-combat");
  }
  run(1000);

  world.end(true);
  run(1600);
  shot("rpg-end");
  return 0;
}

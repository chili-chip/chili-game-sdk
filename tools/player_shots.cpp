// Plays the RPG player app (players/rpg) on the fake 32blit API and writes
// screenshots as PPM files into the folder given as the first argument.
// tools/screenshots.py turns them into PNGs.

#include "../players/rpg/player.cpp"

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

int main(int argc, char **argv) {
  if(argc > 1) out_dir = argv[1];

  fake::reset();
  ::init();
  run(800);
  shot("player-title");

  fake::reset();
  fake::files()["broken.rpg"] = R"({ "format": "chili-rpg", "version": 1, "title": "Broken", "rooms": [] })";
  fake::launch("broken.rpg");
  ::init();
  shot("player-error");
  return 0;
}

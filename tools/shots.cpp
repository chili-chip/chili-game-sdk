// Plays the demo game with scripted input, on the fake 32blit API from the
// tests, and writes screenshots as PPM files into the folder given as the
// first argument. tools/screenshots.py turns them into the docs' PNGs.

#include "../examples/demo/demo.cpp"

#include "fake_api.hpp"

#include <cstdio>

static uint32_t clock_ms = 0;
static const char *out_dir = ".";

// Runs the game for `ms` with `held` buttons down, 10 ms per update like the
// firmware.
static void run(uint32_t ms, uint32_t held = 0) {
  for(uint32_t t = 0; t < ms; t += 10) {
    fake::hold(held);
    clock_ms += 10;
    ::update(clock_ms);
  }
}

// One press: down for a frame, then up for a while.
static void tap(uint32_t button, uint32_t wait_ms = 100) {
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

// Moves the bowl under the chilli until something has been caught.
static void catch_one() {
  uint32_t before = score;
  for(int i = 0; i < 2000 && score == before; i++) {
    uint32_t held = 0;
    if(play.chilli_x < play.bowl - 2) held = Button::DPAD_LEFT;
    if(play.chilli_x > play.bowl + 2) held = Button::DPAD_RIGHT;
    run(10, held);
  }
}

int main(int argc, char **argv) {
  if(argc > 1) out_dir = argv[1];
  fake::reset();
  ::init();

  run(800);
  tap(Button::DPAD_DOWN);
  shot("menu");

  tap(Button::DPAD_DOWN);
  tap(Button::DPAD_DOWN);
  tap(Button::DPAD_DOWN);
  tap(Button::DPAD_DOWN);  // HOW TO PLAY
  tap(Button::A, 500);
  shot("text-screen");
  tap(Button::B, 300);

  tap(Button::DPAD_DOWN);  // back to PLAY
  tap(Button::A, 0);
  run(80);
  shot("fade");
  run(600);

  for(int i = 0; i < 4; i++) catch_one();
  run(120);
  shot("effects");

  tap(Button::MENU, 300);
  tap(Button::DPAD_DOWN);
  shot("pause");
  tap(Button::DPAD_UP);
  tap(Button::A, 300);

  // miss three times
  while(scenes.is(play)) run(10, play.chilli_x < 64 ? Button::DPAD_RIGHT : Button::DPAD_LEFT);
  run(800);
  tap(Button::DPAD_UP);
  tap(Button::DPAD_UP);
  tap(Button::A);
  tap(Button::DPAD_UP);
  shot("name-entry");

  tap(Button::A);
  tap(Button::A, 900);
  shot("score-table");
  return 0;
}

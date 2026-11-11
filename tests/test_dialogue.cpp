#include "check.hpp"
#include "fake_api.hpp"

#include "chili/dialogue.hpp"

#include <string>

using namespace blit;

// One frame: `pressed` goes down for this update only.
static bool frame(chili::TextBox &box, uint32_t dt, uint32_t pressed = 0) {
  return box.update(dt, pressed);
}

TEST(text_appears_letter_by_letter) {
  set_screen_mode(ScreenMode::hires);
  chili::TextBox box;
  box.say("Hello");
  CHECK(box.active());
  CHECK(box.typing());
  frame(box, box.char_ms * 2);
  CHECK(box.typing());
  frame(box, box.char_ms * 10);
  CHECK(!box.typing());
}

TEST(a_shows_the_page_then_closes) {
  set_screen_mode(ScreenMode::hires);
  chili::TextBox box;
  box.say("Hello there, this is a test.");
  frame(box, 300);  // past the input delay, still typing
  CHECK(box.typing());
  CHECK(!frame(box, 10, Button::A));
  CHECK(!box.typing());
  CHECK(frame(box, 10, Button::A));
  CHECK(!box.active());
}

TEST(b_shows_the_page_but_does_not_close) {
  set_screen_mode(ScreenMode::hires);
  chili::TextBox box;
  box.say("Hello there.");
  frame(box, 300);
  frame(box, 10, Button::B);
  CHECK(!box.typing());
  CHECK(!frame(box, 10, Button::B));
  CHECK(box.active());
}

TEST(long_text_is_split_into_pages_of_three_lines) {
  set_screen_mode(ScreenMode::hires);
  chili::TextBox box;
  box.say("one\ntwo\nthree\nfour\nfive");
  CHECK_EQ(box.pages(), 2);
  frame(box, 2000);
  CHECK(!frame(box, 10, Button::A));
  CHECK_EQ(box.page(), 1);
  CHECK(box.typing());
  frame(box, 2000);
  CHECK(frame(box, 10, Button::A));
}

TEST(a_mashed_too_early_is_ignored) {
  set_screen_mode(ScreenMode::hires);
  chili::TextBox box;
  box.say("Hi");
  frame(box, 100, Button::A);
  CHECK(box.typing() || box.active());
  CHECK_EQ(box.page(), 0);
}

TEST(choices_are_picked_with_up_down_and_a) {
  set_screen_mode(ScreenMode::hires);
  chili::TextBox box;
  box.say("Help me?");
  box.ask({"Yes", "No", "Maybe"});
  frame(box, 2000);
  frame(box, 10, Button::DPAD_DOWN);
  frame(box, 10, Button::DPAD_DOWN);
  CHECK(frame(box, 10, Button::A));
  CHECK_EQ(box.choice(), 2);
}

TEST(on_letter_blips_while_typing) {
  set_screen_mode(ScreenMode::hires);
  chili::TextBox box;
  int blips = 0;
  box.on_letter = [&] { blips++; };
  box.say("abcdef");
  for(int i = 0; i < 20; i++) frame(box, box.char_ms);
  CHECK_EQ(blips, 3);
}

TEST(renders_without_a_sheet) {
  set_screen_mode(ScreenMode::hires);
  chili::TextBox box;
  box.say("Hi", "ELDER", 3, nullptr);
  frame(box, 1000);
  box.render(0);
  // the box covers the bottom of the screen
  const uint8_t *px = fake::screen_pixels();
  int y = 127, x = 64;
  CHECK(px[(y * 128 + x) * 3] != 0 || px[(y * 128 + x) * 3 + 1] != 0 || px[(y * 128 + x) * 3 + 2] != 0);
}

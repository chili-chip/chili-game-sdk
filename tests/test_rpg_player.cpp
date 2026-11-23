// The RPG player app (players/rpg): opens the .rpg file it was started with.

#include "../players/rpg/player.cpp"

#include "rpg_world.hpp"

using namespace blit;

static bool has(const std::string &text, const std::string &part) {
  return text.find(part) != std::string::npos;
}

TEST(plays_the_template_when_started_without_a_file) {
  CHECK(open_game().empty());
  CHECK(world.game.title == "Pepper Village");
  CHECK(world.sheet != nullptr);
}

TEST(opens_the_file_it_was_started_with) {
  fake::files()["games/test.rpg"] = fixture::game;
  fake::launch("games/test.rpg");
  CHECK(open_game().empty());
  CHECK(world.game.title == "Test");
}

TEST(says_when_the_file_cannot_be_read) {
  fake::launch("games/gone.rpg");
  CHECK(has(open_game(), "Can't read games/gone.rpg"));
}

TEST(says_what_is_wrong_with_a_broken_file) {
  fake::files()["games/bad.rpg"] = fixture::edit("\"kind\": \"slime\"", "\"kind\": \"blob\"");
  fake::launch("games/bad.rpg");
  CHECK(has(open_game(), "no kind \"blob\""));
}

TEST(init_shows_the_error_instead_of_the_title) {
  fake::files()["games/bad.rpg"] = "{";
  fake::launch("games/bad.rpg");
  ::init();
  CHECK(!error.empty());
  ::render(0);  // draws the error, does not crash
}

TEST(saves_when_quitting_while_playing) {
  ::init();
  go_play(false);
  CHECK(blit::on_quit != nullptr);
  blit::on_quit();
  CHECK(fake::files().count("saves/save0") == 1);
}

TEST(does_not_save_when_quitting_from_the_title) {
  ::init();
  blit::on_quit();
  CHECK(fake::files().count("saves/save0") == 0);
}

TEST(titles_shrink_to_fit_the_screen) {
  set_screen_mode(ScreenMode::hires);
  CHECK_EQ(title_scale("Pip"), 3);
  int s = title_scale("Pepper Village");
  CHECK(s >= 1);
  CHECK(screen.measure_text("Pepper Village", minimal_font, true).w * s <= 120);
  // too long for big letters: plain text, wrapped
  CHECK_EQ(title_scale("The Very Long Quest of Old Pip"), 0);
}

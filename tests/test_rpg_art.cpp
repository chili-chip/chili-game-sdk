#include "rpg_world.hpp"

#include "chili/rpg/game.hpp"

#include <string>

using namespace chili::rpg;

static bool has(const std::string &text, const std::string &part) {
  return text.find(part) != std::string::npos;
}

// Picture 0: colour c (#f4f4f4) with a clear top-left pixel.
// Picture 1: colour 2 (#b13e53).
static const char *art = R"("art": {
    "palette": "sweetie16",
    "pictures": [
      [".ccccccc", "cccccccc", "cccccccc", "cccccccc", "cccccccc", "cccccccc", "cccccccc", "cccccccc"],
      ["22222222", "22222222", "22222222", "22222222", "22222222", "22222222", "22222222", "22222222"]
    ]
  },
  "sprites": "sheet.png",)";

static std::string with_art(const std::string &a = art) {
  return fixture::edit("\"sprites\": \"sheet.png\",", a);
}

static std::string load_error(const std::string &text) {
  Game g;
  return load(text, g);
}

TEST(reads_pictures_into_a_sheet) {
  Game g;
  std::string err = load(with_art(), g);
  if(!err.empty()) std::printf("%s\n", err.c_str());
  CHECK(err.empty());
  CHECK_EQ(g.art_count, 2);
  CHECK_EQ(g.art.size(), ART_SIZE * ART_SIZE);
  CHECK_EQ(g.art[0], ART_CLEAR);
  CHECK_EQ(g.art[1], 12);
  CHECK_EQ(g.art[7 * ART_SIZE + 7], 12);
  CHECK_EQ(g.art[8], 2);           // picture 1 starts at x 8
  CHECK_EQ(g.art[7 * ART_SIZE + 15], 2);
  CHECK_EQ(g.art[16], ART_CLEAR);  // picture 2 is not drawn: clear
}

TEST(no_art_means_no_sheet) {
  Game g;
  CHECK(load(fixture::game, g).empty());
  CHECK_EQ(g.art_count, 0);
  CHECK(g.art.empty());
}

TEST(picture_256_goes_on_the_last_row) {
  std::string pics;
  for(int i = 0; i < 256; i++) {
    if(i) pics += ",";
    pics += i == 255 ? R"(["f.......","........","........","........","........","........","........","........"])"
                     : R"(["........","........","........","........","........","........","........","........"])";
  }
  Game g;
  CHECK(load(with_art("\"art\": { \"pictures\": [" + pics + "] },"), g).empty());
  CHECK_EQ(g.art_count, 256);
  CHECK_EQ(g.art[120 * ART_SIZE + 120], 15);
}

TEST(art_errors_say_where) {
  CHECK(has(load_error(with_art(R"("art": { "palette": "pico8", "pictures": [] },)")),
            "art.palette: only \"sweetie16\""));
  CHECK(has(load_error(with_art(R"("art": { "pictures": {} },)")), "art.pictures: not a list"));
  CHECK(has(load_error(with_art(R"("art": { "pictures": [["........"]] },)")),
            "art.pictures[0]: needs 8 rows"));
  CHECK(has(load_error(with_art(R"("art": { "pictures": [["........","........","........","....","........","........","........","........"]] },)")),
            "art.pictures[0][3]: needs 8 characters"));
  CHECK(has(load_error(with_art(R"("art": { "pictures": [["........","........","........","........","....x...","........","........","........"]] },)")),
            "art.pictures[0][4]: '0' to 'f' or '.'"));
  std::string pics;
  for(int i = 0; i < 257; i++) pics += std::string(i ? "," : "") + R"(["........","........","........","........","........","........","........","........"])";
  CHECK(has(load_error(with_art("\"art\": { \"pictures\": [" + pics + "] },")), "art.pictures: at most 256"));
}

TEST(world_draws_the_art) {
  World w;
  CHECK(play::start(w, with_art()));
  w.render(0);
  const uint8_t *px = fake::screen_pixels();
  // Tile (3,4) is floor, picture 0. Its pixel (1,1) is colour c, its top-left
  // pixel is clear, so the background shows.
  int x = 3 * 8, y = 4 * 8;
  const uint8_t *in = px + ((y + 1) * 128 + x + 1) * 3;
  CHECK_EQ(in[0], 0xf4);
  CHECK_EQ(in[1], 0xf4);
  CHECK_EQ(in[2], 0xf4);
  const uint8_t *corner = px + (y * 128 + x) * 3;
  CHECK_EQ(corner[0], 0x22);
  CHECK_EQ(corner[1], 0x23);
  CHECK_EQ(corner[2], 0x23);
  // Tile (0,4) is wall, picture 1.
  const uint8_t *wall = px + ((y + 2) * 128 + 2) * 3;
  CHECK_EQ(wall[0], 0xb1);
  CHECK_EQ(wall[1], 0x3e);
  CHECK_EQ(wall[2], 0x53);
}

TEST(art_wins_over_a_sheet_set_in_code) {
  World w;
  blit::Surface other(nullptr, blit::PixelFormat::RGB, blit::Size(0, 0));
  w.sheet = &other;
  CHECK(play::start(w, with_art()));
  CHECK(w.sheet != &other);
}

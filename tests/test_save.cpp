#include "check.hpp"
#include "fake_api.hpp"

#include "chili/save.hpp"

#include <cstring>

struct Settings {
  uint8_t speed = 1;
  uint8_t music = 1;
  char name[4] = "AAA";
  uint32_t best = 0;
};

struct Bigger {
  uint8_t speed = 1;
  uint8_t music = 1;
  char name[4] = "AAA";
  uint32_t best = 0;
  uint32_t extra = 0;
};

const uint32_t MAGIC = 0x54534554;  // "TEST"

TEST(missing_save_gives_defaults) {
  chili::Save<Settings> save(MAGIC, 1);
  save.data.speed = 9;
  CHECK(!save.load());
  CHECK_EQ(save.data.speed, 1);
  CHECK(strcmp(save.data.name, "AAA") == 0);
}

TEST(store_then_load_round_trips) {
  chili::Save<Settings> a(MAGIC, 1);
  a.data.speed = 2;
  a.data.best = 1234;
  memcpy(a.data.name, "KAC", 4);
  a.store();

  chili::Save<Settings> b(MAGIC, 1);
  CHECK(b.load());
  CHECK_EQ(b.data.speed, 2);
  CHECK_EQ(b.data.best, uint32_t(1234));
  CHECK(strcmp(b.data.name, "KAC") == 0);
}

TEST(other_magic_gives_defaults) {
  chili::Save<Settings> a(MAGIC, 1);
  a.data.best = 5;
  a.store();
  chili::Save<Settings> b(MAGIC + 1, 1);
  CHECK(!b.load());
  CHECK_EQ(b.data.best, uint32_t(0));
}

TEST(old_version_gives_defaults) {
  chili::Save<Settings> a(MAGIC, 1);
  a.data.best = 5;
  a.store();
  chili::Save<Settings> b(MAGIC, 2);
  CHECK(!b.load());
  CHECK_EQ(b.data.best, uint32_t(0));
}

TEST(changed_struct_without_new_version_gives_defaults) {
  chili::Save<Bigger> big(MAGIC, 1);
  big.data.best = 5;
  big.store();
  chili::Save<Settings> small(MAGIC, 1);
  CHECK(!small.load());
  CHECK_EQ(small.data.best, uint32_t(0));
}

TEST(slots_are_separate_files) {
  chili::Save<Settings> a(MAGIC, 1, 0);
  chili::Save<Settings> b(MAGIC, 1, 1);
  a.data.best = 1;
  b.data.best = 2;
  a.store();
  b.store();
  CHECK_EQ(fake::files().size(), size_t(2));
  chili::Save<Settings> c(MAGIC, 1, 1);
  CHECK(c.load());
  CHECK_EQ(c.data.best, uint32_t(2));
}

TEST(store_on_quit_writes_when_the_game_quits) {
  chili::Save<Settings> a(MAGIC, 1);
  a.store_on_quit();
  CHECK(blit::on_quit != nullptr);
  a.data.best = 77;
  CHECK(fake::files().empty());
  blit::on_quit();
  chili::Save<Settings> b(MAGIC, 1);
  CHECK(b.load());
  CHECK_EQ(b.data.best, uint32_t(77));
}

TEST(reset_restores_defaults) {
  chili::Save<Settings> a(MAGIC, 1);
  a.data.speed = 0;
  a.reset();
  CHECK_EQ(a.data.speed, 1);
}

#include "check.hpp"
#include "fake_api.hpp"

#include "chili/scores.hpp"

#include <cstring>

struct Entry {
  char name[4];
  uint32_t score;
};

static Entry make(const char *name, uint32_t score) {
  Entry e{};
  memcpy(e.name, name, 3);
  e.score = score;
  return e;
}

TEST(empty_table_ranks_any_score_first) {
  chili::HighScores<Entry, 5> t{};
  CHECK_EQ(t.rank(10), 0);
  CHECK_EQ(t.rank(0), -1);  // zero never makes the table
  CHECK_EQ(t.best().score, uint32_t(0));
}

TEST(insert_pushes_lower_entries_down) {
  chili::HighScores<Entry, 3> t{};
  t.insert(0, make("AAA", 300));
  t.insert(1, make("BBB", 100));
  t.insert(t.rank(200), make("CCC", 200));
  CHECK_EQ(t[0].score, uint32_t(300));
  CHECK_EQ(t[1].score, uint32_t(200));
  CHECK(strcmp(t[1].name, "CCC") == 0);
  CHECK_EQ(t[2].score, uint32_t(100));
}

TEST(full_table_drops_the_last_entry) {
  chili::HighScores<Entry, 2> t{};
  t.add(make("AAA", 300));
  t.add(make("BBB", 200));
  CHECK_EQ(t.rank(100), -1);
  CHECK_EQ(t.add(make("CCC", 250)), 1);
  CHECK_EQ(t[1].score, uint32_t(250));
  CHECK_EQ(t.add(make("DDD", 10)), -1);
}

TEST(ties_rank_below_the_older_score) {
  chili::HighScores<Entry, 3> t{};
  t.add(make("OLD", 100));
  CHECK_EQ(t.rank(100), 1);
}

TEST(insert_out_of_range_is_ignored) {
  chili::HighScores<Entry, 2> t{};
  t.insert(-1, make("AAA", 1));
  t.insert(2, make("AAA", 1));
  CHECK_EQ(t[0].score, uint32_t(0));
  CHECK_EQ(t[1].score, uint32_t(0));
}

TEST(lower_is_better_for_times) {
  chili::HighScores<Entry, 3, chili::Order::lower_first> t{};
  CHECK_EQ(t.add(make("AAA", 90)), 0);
  CHECK_EQ(t.add(make("BBB", 60)), 0);
  CHECK_EQ(t.add(make("CCC", 75)), 1);
  CHECK_EQ(t.best().score, uint32_t(60));
  CHECK_EQ(t[2].score, uint32_t(90));
  CHECK_EQ(t.rank(100), -1);
  CHECK_EQ(t.rank(0), -1);
}

TEST(table_is_plain_data_for_a_save) {
  // The table goes straight into a save struct, so it must stay trivially
  // copyable and exactly the size of its entries.
  CHECK((std::is_trivially_copyable_v<chili::HighScores<Entry, 5>>));
  CHECK_EQ(sizeof(chili::HighScores<Entry, 5>), sizeof(Entry) * 5);
}

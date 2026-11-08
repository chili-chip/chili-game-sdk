#pragma once

#include <cstdint>

namespace chili {

enum class Order : uint8_t {
  higher_first,  // points
  lower_first,   // times
};

// A top-N high-score table that lives inside your save struct.
//
// `Entry` is your own plain struct with a `uint32_t score` field (and
// usually `char name[4]`). A score of 0 means an empty slot, and never makes
// the table. A tie goes below the score that was there first.
//
//   struct ScoreEntry { char name[4]; uint32_t score; };
//   struct SaveData { chili::HighScores<ScoreEntry, 5> scores; };
template<typename Entry, int N, Order order = Order::higher_first>
struct HighScores {
  static_assert(N > 0, "a score table needs at least one slot");

  Entry entries[N];

  static constexpr int size = N;

  // The place (0 = best) `score` would take, or -1 when it does not make it.
  int rank(uint32_t score) const {
    if(score == 0) return -1;
    for(int i = 0; i < N; i++) {
      uint32_t there = entries[i].score;
      if(there == 0 || beats(score, there)) return i;
    }
    return -1;
  }

  // Puts `entry` at `place`, pushing the entries below it down one. The last
  // entry falls off. Out-of-range places are ignored.
  void insert(int place, const Entry &entry) {
    if(place < 0 || place >= N) return;
    for(int i = N - 1; i > place; i--) entries[i] = entries[i - 1];
    entries[place] = entry;
  }

  // rank() and insert() in one go. Returns the place, or -1.
  int add(const Entry &entry) {
    int place = rank(entry.score);
    insert(place, entry);
    return place;
  }

  const Entry &best() const { return entries[0]; }
  const Entry &operator[](int i) const { return entries[i]; }
  Entry &operator[](int i) { return entries[i]; }

private:
  static bool beats(uint32_t a, uint32_t b) {
    return order == Order::higher_first ? a > b : a < b;
  }
};

}

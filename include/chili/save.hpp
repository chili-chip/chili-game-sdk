#pragma once

#include "32blit.hpp"

#include <cstdint>
#include <type_traits>

namespace chili {

// A save file holding one struct, with a magic number and a version.
//
// Give the struct's fields default values: those are what a new player gets.
// load() falls back to them when there is no save, when it belongs to
// another game (magic), when it is from an older version, or when the struct
// changed size. Bump the version whenever you change the struct.
//
//   struct Settings { uint8_t music = 1; uint32_t best = 0; };
//   chili::Save<Settings> save(0x4b4e5332, 1);
//   save.load();
//   save.store_on_quit();
template<typename T>
class Save {
  static_assert(std::is_trivially_copyable_v<T>, "save data must be plain data (no pointers, strings or vectors)");
  static_assert(sizeof(T) <= 0xffff, "save data must be under 64 KB");

public:
  Save(uint32_t magic, uint16_t version, int slot = 0) : magic(magic), version(version), slot(slot) {}

  T data{};

  // Reads the save. Returns true when it was valid; otherwise `data` is reset
  // to the defaults and false is returned.
  bool load() {
    Stored stored;
    if(blit::read_save(stored, slot) && stored.magic == magic && stored.version == version &&
       stored.size == sizeof(T)) {
      data = stored.data;
      return true;
    }
    reset();
    return false;
  }

  // Writes `data` to the save file.
  void store() const {
    Stored stored{magic, version, uint16_t(sizeof(T)), data};
    blit::write_save(stored, slot);
  }

  // Sets `data` back to the defaults. Does not write.
  void reset() { data = T{}; }

  // Stores the save when the player quits (system menu, HOME, or closing the
  // window). This sets blit::on_quit, so use it for one save only.
  void store_on_quit() {
    quitting = this;
    blit::on_quit = [] {
      if(quitting) quitting->store();
    };
  }

private:
  struct Stored {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    T data;
  };

  static inline Save *quitting = nullptr;

  uint32_t magic;
  uint16_t version;
  int slot;
};

}

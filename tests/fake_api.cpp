#include "fake_api.hpp"

#include "engine/api_private.hpp"

#include <cstring>

using namespace blit;

namespace {

AudioChannel fake_channels[CHANNEL_COUNT];
std::map<std::string, std::string> fake_files;
uint32_t fake_now = 0;
uint32_t fake_seed = 1;

struct Handle {
  std::string path;
};

void *open_file(const std::string &path, int mode) {
  if(mode & OpenMode::write) {
    fake_files[path];
  } else if(!fake_files.count(path)) {
    return nullptr;
  }
  return new Handle{path};
}

int32_t read_file(void *fh, uint32_t offset, uint32_t length, char *buffer) {
  auto &data = fake_files[static_cast<Handle *>(fh)->path];
  if(offset >= data.size()) return 0;
  uint32_t n = std::min<uint32_t>(length, data.size() - offset);
  memcpy(buffer, data.data() + offset, n);
  return n;
}

int32_t write_file(void *fh, uint32_t offset, uint32_t length, const char *buffer) {
  auto &data = fake_files[static_cast<Handle *>(fh)->path];
  if(data.size() < offset + length) data.resize(offset + length);
  memcpy(data.data() + offset, buffer, length);
  return length;
}

int32_t close_file(void *fh) {
  delete static_cast<Handle *>(fh);
  return 0;
}

uint32_t get_file_length(void *fh) {
  return fake_files[static_cast<Handle *>(fh)->path].size();
}

bool file_exists(const std::string &path) {
  return fake_files.count(path) > 0;
}

const char *get_save_path() {
  return "saves/";
}

uint32_t now() {
  return fake_now;
}

uint32_t random() {
  fake_seed = fake_seed * 1103515245u + 12345u;
  return fake_seed >> 8;
}

void debug(const char *) {}

// A 128x128 RGB screen, like the console's.
uint8_t screen_buf[128 * 128 * 3];

bool set_screen_mode_format(ScreenMode, SurfaceTemplate &t) {
  t.data = screen_buf;
  t.bounds = Size(128, 128);
  t.format = PixelFormat::RGB;
  return true;
}

APIConst make_api() {
  APIConst a{};
  a.channels = fake_channels;
  a.now = now;
  a.random = random;
  a.debug = debug;
  a.set_screen_mode_format = set_screen_mode_format;
  a.open_file = open_file;
  a.read_file = read_file;
  a.write_file = write_file;
  a.close_file = close_file;
  a.get_file_length = get_file_length;
  a.file_exists = file_exists;
  a.get_save_path = get_save_path;
  return a;
}

APIConst fake_api_const = make_api();
APIData fake_api_data;

}

namespace blit {
const APIConst &api = fake_api_const;
APIData &api_data = fake_api_data;
}

namespace fake {

void reset() {
  fake_files.clear();
  for(auto &c : fake_channels) c = AudioChannel();
  fake_now = 0;
  fake_seed = 1;
  api_data.buttons.state = 0;
  api_data.buttons.pressed = 0;
  api_data.buttons.released = 0;
  blit::on_quit = nullptr;
}

void hold(uint32_t held) {
  uint32_t before = api_data.buttons.state;
  api_data.buttons.state = held;
  api_data.buttons.pressed = held & ~before;
  api_data.buttons.released = before & ~held;
}

const uint8_t *screen_pixels() {
  return screen_buf;
}

std::map<std::string, std::string> &files() {
  return fake_files;
}

}

#include "chili/audio.hpp"

#include "32blit.hpp"

#include <algorithm>
#include <cmath>

using namespace blit;

namespace chili::audio {

// ---------------------------------------------------------------------------
// Parser

void parse_track(const char *text, std::vector<uint8_t> &out) {
  static const int8_t semis[] = {9, 11, 0, 2, 4, 5, 7};  // A B C D E F G
  const char *p = text ? text : "";
  while(*p) {
    while(*p == ' ') p++;
    if(!*p) break;

    const char *tok = p;
    while(*p && *p != ' ') p++;
    size_t n = p - tok;

    char c = tok[0];
    if(c == '-') out.push_back(HOLD);
    else if(c == '.') out.push_back(REST);
    else if(c == 'K') out.push_back(KICK);
    else if(c == 'S') out.push_back(SNARE);
    else if(c == 'H') out.push_back(HAT);
    else if(c >= 'A' && c <= 'G') {
      int semi = semis[c - 'A'];
      size_t i = 1;
      if(i < n && tok[i] == '#') {
        semi++;
        i++;
      }
      int octave = (i < n && tok[i] >= '0' && tok[i] <= '9') ? tok[i] - '0' : 4;
      out.push_back(uint8_t(12 * (octave + 1) + semi));
    } else {
      out.push_back(REST);
    }
  }
}

uint16_t note_hz(uint8_t note) {
  return uint16_t(440.0f * powf(2.0f, (note - 69) / 12.0f) + 0.5f);
}

// ---------------------------------------------------------------------------
// Music

namespace {

const Song *song = nullptr;
std::vector<uint8_t> lead, bass, drums;
size_t length = 0;
size_t step_index = 0;
float step_timer = 0;
float tempo = 1.0f;
bool paused = false;
bool done = false;
bool music = true;
bool sound = true;
uint32_t last_time = 0;
float kick_freq = 0;

void setup_music_channels() {
  AudioChannel &l = channels[LEAD];
  l.waveforms = Waveform::SQUARE;
  l.pulse_width = 0x4000;  // 25% duty for a classic chip lead
  l.attack_ms = 4;
  l.decay_ms = 140;
  l.sustain = 0x9000;
  l.release_ms = 60;
  l.volume = 0x3800;

  AudioChannel &b = channels[BASS];
  b.waveforms = Waveform::TRIANGLE;
  b.attack_ms = 2;
  b.decay_ms = 90;
  b.sustain = 0xa000;
  b.release_ms = 30;
  b.volume = 0x7000;

  AudioChannel &k = channels[KICK_CHANNEL];
  k.waveforms = Waveform::SINE;
  k.attack_ms = 1;
  k.decay_ms = 90;
  k.sustain = 0;
  k.release_ms = 10;
  k.volume = 0x9000;

  AudioChannel &n = channels[NOISE];
  n.waveforms = Waveform::NOISE;
  n.attack_ms = 1;
  n.sustain = 0;
  n.release_ms = 5;
}

void release_music_channels() {
  for(int c = LEAD; c <= NOISE; c++) channels[c].trigger_release();
}

void play_melodic(AudioChannel &ch, uint8_t code) {
  if(code == HOLD) return;
  if(code < 12) {
    ch.trigger_release();
    return;
  }
  ch.frequency = note_hz(code);
  ch.trigger_attack();
}

void play_drum(uint8_t code) {
  if(code == KICK) {
    kick_freq = 160;
    channels[KICK_CHANNEL].frequency = uint16_t(kick_freq);
    channels[KICK_CHANNEL].trigger_attack();
  } else if(code == SNARE) {
    AudioChannel &n = channels[NOISE];
    n.decay_ms = 110;
    n.volume = 0x3000;
    n.trigger_attack();
  } else if(code == HAT) {
    AudioChannel &n = channels[NOISE];
    n.decay_ms = 25;
    n.volume = 0x1400;
    n.trigger_attack();
  }
}

uint8_t at(const std::vector<uint8_t> &t, size_t i) {
  return i < t.size() ? t[i] : uint8_t(REST);
}

void play_step() {
  play_melodic(channels[LEAD], at(lead, step_index));
  play_melodic(channels[BASS], at(bass, step_index));
  play_drum(at(drums, step_index));
}

// ---------------------------------------------------------------------------
// Sound effects

struct SfxPlayer {
  const Sfx *sfx = nullptr;
  float pitch = 1.0f;
  uint8_t step = 0;
  int32_t timer = 0;
};

SfxPlayer players[CHANNEL_COUNT];

uint16_t scaled(uint16_t freq, float pitch) {
  return uint16_t(std::min(65535.0f, freq * pitch + 0.5f));
}

void update_sfx(uint32_t dt) {
  for(int c = 0; c < CHANNEL_COUNT; c++) {
    SfxPlayer &pl = players[c];
    if(!pl.sfx) continue;
    pl.timer -= dt;
    if(pl.timer > 0) continue;

    AudioChannel &ch = channels[c];
    pl.step++;
    if(pl.step >= pl.sfx->count) {
      ch.trigger_release();
      pl.sfx = nullptr;
      continue;
    }
    ch.frequency = scaled(pl.sfx->steps[pl.step].freq, pl.pitch);
    pl.timer += pl.sfx->steps[pl.step].ms;
  }
}

}

void init() {
  song = nullptr;
  length = step_index = 0;
  paused = done = false;
  last_time = 0;
  kick_freq = 0;
  for(auto &pl : players) pl = SfxPlayer();
  setup_music_channels();
}

void update(uint32_t time) {
  uint32_t dt = last_time ? time - last_time : 0;
  last_time = time;
  if(dt > 100) dt = 100;  // don't race to catch up after a stall

  update_sfx(dt);

  if(kick_freq > 45) {
    kick_freq *= 0.82f;
    channels[KICK_CHANNEL].frequency = uint16_t(kick_freq);
  }

  if(!song || paused || done) return;

  step_timer -= dt;
  while(step_timer <= 0) {
    step_index++;
    if(step_index >= length) {
      if(!song->loop) {
        done = true;
        release_music_channels();
        return;
      }
      step_index = 0;
    }
    if(music) play_step();
    step_timer += song->step_ms * tempo;
  }
}

void play(const Song &s) {
  release_music_channels();
  song = &s;
  paused = false;
  done = false;
  tempo = 1.0f;

  lead.clear();
  bass.clear();
  drums.clear();
  parse_track(s.lead, lead);
  parse_track(s.bass, bass);
  parse_track(s.drums, drums);
  length = std::max(lead.size(), std::max(bass.size(), drums.size()));
  if(length == 0) {
    done = true;
    return;
  }

  setup_music_channels();
  step_index = 0;
  step_timer = s.step_ms;
  if(music) play_step();
}

void stop() {
  release_music_channels();
  song = nullptr;
  done = false;
}

void pause(bool p) {
  if(p && !paused) release_music_channels();
  paused = p;
}

const Song *current() {
  return song;
}

bool finished() {
  return !song || done;
}

void set_tempo(float scale) {
  tempo = scale;
}

void play_sfx(const Sfx &s, float pitch) {
  if(!sound || s.count == 0 || s.channel >= CHANNEL_COUNT) return;
  SfxPlayer &pl = players[s.channel];
  if(pl.sfx && s.priority < pl.sfx->priority) return;

  AudioChannel &ch = channels[s.channel];
  ch.off();
  ch.waveforms = s.waveform;
  ch.volume = s.volume;
  ch.pulse_width = s.pulse_width;
  ch.attack_ms = 2;
  ch.decay_ms = 10;
  ch.sustain = 0xd000;
  ch.release_ms = s.release_ms;
  ch.frequency = scaled(s.steps[0].freq, pitch);
  ch.trigger_attack();

  pl.sfx = &s;
  pl.pitch = pitch;
  pl.step = 0;
  pl.timer = s.steps[0].ms;
}

void set_music(bool on) {
  music = on;
  if(!on) release_music_channels();
}

void set_sound(bool on) {
  sound = on;
  if(on) return;
  for(int c = 0; c < CHANNEL_COUNT; c++) {
    if(players[c].sfx) channels[c].trigger_release();
    players[c].sfx = nullptr;
  }
}

bool music_on() {
  return music;
}

bool sound_on() {
  return sound;
}

}

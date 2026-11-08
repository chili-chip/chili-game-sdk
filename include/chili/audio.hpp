#pragma once

#include <cstdint>
#include <vector>

// A chiptune player for songs written as note strings, and short sound
// effects. Everything plays on the 32blit audio channels:
//
// | Channel | Use                              |
// | ------- | -------------------------------- |
// | 0       | lead (square, 25% duty)          |
// | 1       | bass (triangle)                  |
// | 2       | kick drum (sine with a pitch drop) |
// | 3       | snare and hi-hat (noise)         |
// | 4-7     | sound effects                    |
//
// There is no volume here: the console's system menu owns it.
namespace chili::audio {

constexpr uint8_t LEAD = 0;
constexpr uint8_t BASS = 1;
constexpr uint8_t KICK_CHANNEL = 2;
constexpr uint8_t NOISE = 3;
constexpr uint8_t SFX1 = 4;
constexpr uint8_t SFX2 = 5;
constexpr uint8_t SFX3 = 6;
constexpr uint8_t SFX4 = 7;

// A song: three tracks of 16th-note steps separated by spaces.
//
//   C4, F#5   start a note (octave 0-9, 4 if left out)
//   -         hold the previous note
//   .         rest
//   K S H     kick, snare, hi-hat (drum track)
//
// A track can be empty (""). The song lasts as long as its longest track.
struct Song {
  uint16_t step_ms;    // length of one step
  bool loop;           // false: plays once, then finished() is true
  const char *lead;
  const char *bass;
  const char *drums;
};

// One step of a sound effect: a frequency for some milliseconds.
struct SfxStep {
  uint16_t freq;
  uint16_t ms;
};

// A short sound effect on one channel.
struct Sfx {
  uint8_t channel;      // SFX1 to SFX4
  uint8_t waveform;     // blit::Waveform
  uint16_t volume;
  uint16_t pulse_width;
  uint16_t release_ms;
  uint8_t count;        // steps used
  SfxStep steps[8];
  // A playing effect is only cut off by one with the same or higher priority.
  uint8_t priority = 0;
};

// Sets up the music channels. Call once from init().
void init();
// Call from blit's update() with its time in ms.
void update(uint32_t time);

// Starts a song from the beginning. Keep the Song alive while it plays.
void play(const Song &song);
void stop();
void pause(bool paused);
// The song playing (or done playing), or nullptr after stop().
const Song *current();
// True when a one-shot song has played to the end, or nothing is playing.
bool finished();
// Scales the step length: 0.8 plays 25% faster. play() resets it to 1.
void set_tempo(float scale);

// Plays an effect, with its frequencies scaled by `pitch`.
void play_sfx(const Sfx &sfx, float pitch = 1.0f);

// Music on/off and sound effects on/off (game settings).
void set_music(bool on);
void set_sound(bool on);
bool music_on();
bool sound_on();

// The note-string parser and note table, used by play().
enum Step : uint8_t { REST = 0, HOLD = 1, KICK = 2, SNARE = 3, HAT = 4 };
// Appends one code per step to `out`: a MIDI note number (12 and up), or a
// Step. Unknown tokens become REST.
void parse_track(const char *text, std::vector<uint8_t> &out);
uint16_t note_hz(uint8_t midi_note);

}

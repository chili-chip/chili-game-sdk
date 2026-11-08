#include "check.hpp"
#include "fake_api.hpp"

#include "chili/audio.hpp"

#include <vector>

using namespace blit;
namespace audio = chili::audio;

static std::vector<uint8_t> parse(const char *s) {
  std::vector<uint8_t> out;
  audio::parse_track(s, out);
  return out;
}

TEST(parses_notes_as_midi_numbers) {
  auto t = parse("C4 A4 F#5 C0 B7");
  CHECK_EQ(t.size(), size_t(5));
  CHECK_EQ(t[0], 60);
  CHECK_EQ(t[1], 69);
  CHECK_EQ(t[2], 78);
  CHECK_EQ(t[3], 12);
  CHECK_EQ(t[4], 107);
}

TEST(note_without_octave_is_octave_4) {
  auto t = parse("C G#");
  CHECK_EQ(t[0], 60);
  CHECK_EQ(t[1], 68);
}

TEST(parses_hold_rest_and_drums) {
  auto t = parse("C4 - . K S H");
  CHECK_EQ(t.size(), size_t(6));
  CHECK_EQ(t[1], audio::HOLD);
  CHECK_EQ(t[2], audio::REST);
  CHECK_EQ(t[3], audio::KICK);
  CHECK_EQ(t[4], audio::SNARE);
  CHECK_EQ(t[5], audio::HAT);
}

TEST(extra_spaces_and_unknown_tokens) {
  auto t = parse("  C4   x Z9  ");
  CHECK_EQ(t.size(), size_t(3));
  CHECK_EQ(t[0], 60);
  CHECK_EQ(t[1], audio::REST);
  CHECK_EQ(t[2], audio::REST);
  CHECK(parse("").empty());
}

TEST(note_frequencies) {
  CHECK_EQ(audio::note_hz(69), 440);
  CHECK_EQ(audio::note_hz(81), 880);
  CHECK_EQ(audio::note_hz(60), 262);
}

static const audio::Song tune = {100, false, "C4 - E4 .", "C2 - - -", "K . S ."};
static const audio::Song loop = {100, true, "C4 E4 G4", "", ""};

TEST(play_starts_the_first_step) {
  audio::init();
  audio::play(tune);
  CHECK(audio::current() == &tune);
  CHECK(!audio::finished());
  CHECK_EQ(channels[audio::LEAD].frequency, 262);
  CHECK(channels[audio::LEAD].adsr_phase == ADSRPhase::ATTACK);
  CHECK_EQ(channels[audio::BASS].frequency, 65);
  CHECK(channels[audio::KICK_CHANNEL].adsr_phase == ADSRPhase::ATTACK);
}

TEST(steps_advance_with_time) {
  audio::init();
  audio::update(1000);
  audio::play(tune);
  audio::update(1050);
  CHECK_EQ(channels[audio::LEAD].frequency, 262);
  audio::update(1100);  // step 2: hold
  CHECK_EQ(channels[audio::LEAD].frequency, 262);
  audio::update(1200);  // step 3: E4
  CHECK_EQ(channels[audio::LEAD].frequency, 330);
  audio::update(1300);  // step 4: rest
  CHECK(channels[audio::LEAD].adsr_phase == ADSRPhase::RELEASE);
}

TEST(one_shot_song_finishes) {
  audio::init();
  audio::update(1000);
  audio::play(tune);
  for(uint32_t t = 1000; t <= 1390; t += 10) audio::update(t);
  CHECK(!audio::finished());
  audio::update(1400);
  CHECK(audio::finished());
}

TEST(looping_song_never_finishes) {
  audio::init();
  audio::update(1000);
  audio::play(loop);
  for(uint32_t t = 1000; t <= 3000; t += 10) audio::update(t);
  CHECK(!audio::finished());
}

TEST(tempo_scale_speeds_up_steps) {
  audio::init();
  audio::update(1000);
  audio::play(loop);
  audio::set_tempo(0.5f);  // the step already playing keeps its length
  audio::update(1100);
  CHECK_EQ(channels[audio::LEAD].frequency, 330);
  audio::update(1150);
  CHECK_EQ(channels[audio::LEAD].frequency, 392);
}

TEST(pause_holds_the_song) {
  audio::init();
  audio::update(1000);
  audio::play(tune);
  audio::pause(true);
  for(uint32_t t = 1000; t <= 2000; t += 10) audio::update(t);
  CHECK(!audio::finished());
  audio::pause(false);
  for(uint32_t t = 2000; t <= 2400; t += 10) audio::update(t);
  CHECK(audio::finished());
}

TEST(music_off_keeps_time_but_plays_nothing) {
  audio::init();
  audio::set_music(false);
  audio::update(1000);
  audio::play(tune);
  CHECK(channels[audio::LEAD].adsr_phase != ADSRPhase::ATTACK);
  for(uint32_t t = 1000; t <= 1400; t += 10) audio::update(t);
  CHECK(audio::finished());
  audio::set_music(true);
}

TEST(stop_ends_the_song) {
  audio::init();
  audio::play(loop);
  audio::stop();
  CHECK(audio::current() == nullptr);
  CHECK(audio::finished());
}

static const audio::Sfx blip = {audio::SFX1, Waveform::SQUARE, 0x3000, 0x7fff, 40, 2, {{660, 40}, {990, 70}}};
static const audio::Sfx boom = {audio::SFX1, Waveform::SAW, 0x4800, 0x7fff, 120, 1, {{440, 60}}, 1};

TEST(sfx_plays_its_steps_then_releases) {
  audio::init();
  audio::update(1000);
  audio::play_sfx(blip);
  CHECK_EQ(channels[audio::SFX1].frequency, 660);
  CHECK(channels[audio::SFX1].adsr_phase == ADSRPhase::ATTACK);
  audio::update(1040);
  CHECK_EQ(channels[audio::SFX1].frequency, 990);
  audio::update(1110);
  CHECK(channels[audio::SFX1].adsr_phase == ADSRPhase::RELEASE);
}

TEST(sfx_pitch_scale) {
  audio::init();
  audio::play_sfx(blip, 2.0f);
  CHECK_EQ(channels[audio::SFX1].frequency, 1320);
}

TEST(lower_priority_sfx_does_not_cut_off_a_higher_one) {
  audio::init();
  audio::update(1000);
  audio::play_sfx(boom);
  audio::play_sfx(blip);
  CHECK_EQ(channels[audio::SFX1].frequency, 440);
  audio::update(1060);  // boom done
  audio::play_sfx(blip);
  CHECK_EQ(channels[audio::SFX1].frequency, 660);
}

TEST(sound_off_mutes_sfx) {
  audio::init();
  audio::set_sound(false);
  audio::play_sfx(blip);
  CHECK(channels[audio::SFX1].adsr_phase == ADSRPhase::OFF);
  CHECK(!audio::sound_on());
  audio::set_sound(true);
}

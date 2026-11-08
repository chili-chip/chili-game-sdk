# Music and sound

```cpp
#include "chili/audio.hpp"
```

A chiptune player for songs written as text, and short sound effects. It is the player from Snek, made shared.

## Channels

| Channel | Constant | Plays |
| --- | --- | --- |
| 0 | `LEAD` | melody: square wave, 25% duty |
| 1 | `BASS` | bass: triangle wave |
| 2 | `KICK_CHANNEL` | kick drum: sine with a fast pitch drop |
| 3 | `NOISE` | snare and hi-hat: noise |
| 4-7 | `SFX1`-`SFX4` | sound effects |

There is no volume control: the console's system menu owns volume. Games offer music on/off and sound on/off.

## Set up

```cpp
void init() {
  chili::audio::init();
  chili::audio::set_music(save.data.music);
  chili::audio::set_sound(save.data.sound);
}

void update(uint32_t time) {
  chili::audio::update(time);
}
```

## Songs

A song is three tracks of steps. One step is a 16th note, `step_ms` long.

```cpp
const chili::audio::Song title_song = {
  140,   // step_ms
  true,  // loop
  // lead
  "A4 - - - C5 - E5 - D5 - C5 - B4 - C5 - "
  "A4 - - - F4 - A4 - C5 - - - . . . . ",
  // bass
  "A2 - - - E3 - - - A3 - - - E3 - - - "
  "F2 - - - C3 - - - F3 - - - C3 - - - ",
  // drums
  "K . . . H . . . S . . . H . . H "
  "K . . . H . . . S . . . H . S S ",
};

chili::audio::play(title_song);
```

### Note format

Steps are separated by spaces.

| Token | Means | Example |
| --- | --- | --- |
| Note | starts a note: letter A-G, optional `#`, octave 0-9 (4 if left out) | `C4`, `F#5`, `G` |
| `-` | holds the previous note | `C4 - - -` is one long C |
| `.` | rest: releases the note | `C4 . C4 .` |
| `K` | kick drum (drum track) | `K . . .` |
| `S` | snare | `. . S .` |
| `H` | hi-hat | `H H H H` |

- `A4` is 440 Hz. Octave 4 starts at middle C (`C4`, 262 Hz).
- A track can be empty (`""`). The song is as long as its longest track; shorter tracks rest at the end.
- Anything else reads as a rest.

!!! tip "Keep tracks lined up"
    Write 16 steps per line, one bar of 4/4. Lines that line up across the three tracks are much easier to fix.

### Control

| Function | Does |
| --- | --- |
| `play(const Song &)` | Starts a song from the top. Keep the `Song` alive (a global `const`). |
| `stop()` | Stops. `current()` becomes `nullptr`. |
| `pause(bool)` | Pauses and resumes, for a pause menu. |
| `finished()` | True when a one-shot song (`loop = false`) has ended, or nothing plays. |
| `current()` | The song playing, or `nullptr`. |
| `set_tempo(float scale)` | Step length scale: `0.8` plays 25% faster. `play()` resets it to 1. |

Speed the music up with the game, like Snek does:

```cpp
float progress = float(start_ms - step_ms()) / float(start_ms - min_ms);
chili::audio::set_tempo(1.0f - 0.2f * progress);
```

Play a jingle, then move on when it ends:

```cpp
chili::audio::play(fanfare);   // loop = false
// later, in update():
if(chili::audio::finished()) scenes.go(results);
```

## Sound effects

An effect is a list of up to 8 steps, each a frequency for some milliseconds, on one channel.

```cpp
using namespace blit;
namespace audio = chili::audio;

//                              channel      waveform        volume  pulse   release steps
const audio::Sfx sfx_eat     = {audio::SFX1, Waveform::SQUARE, 0x4000, 0x4000, 50, 4, {{523, 30}, {659, 30}, {784, 30}, {1047, 70}}};
const audio::Sfx sfx_menu    = {audio::SFX2, Waveform::SQUARE, 0x2800, 0x7fff, 20, 1, {{880, 30}}};
const audio::Sfx sfx_die     = {audio::SFX1, Waveform::SAW,    0x4800, 0x7fff, 120, 3, {{440, 60}, {330, 60}, {220, 160}}, 1};
//                                                                                                       priority ^

audio::play_sfx(sfx_eat);
audio::play_sfx(sfx_eat, 1.5f);  // pitch up: for combos
```

| Field | Means |
| --- | --- |
| `channel` | `SFX1` to `SFX4`. Put game sounds and UI sounds on different channels so they do not cut each other off. |
| `waveform` | `blit::Waveform::SQUARE`, `SAW`, `TRIANGLE`, `SINE` or `NOISE`. |
| `volume` | 0 to 0xffff. |
| `pulse_width` | Square duty: `0x7fff` is 50%, `0x4000` 25%. |
| `release_ms` | Fade-out after the last step. |
| `count`, `steps` | How many steps, and the steps (`{freq_hz, ms}`). |
| `priority` | Optional, default 0. A playing effect is only cut off by one of the same or higher priority. Give the death sound 1 so a menu blip cannot cut it off. |

## Settings

| Function | Does |
| --- | --- |
| `set_music(bool)` | Music on/off. The song keeps time while off, so `finished()` still works. |
| `set_sound(bool)` | Effects on/off. |
| `music_on()`, `sound_on()` | The current settings. |

Wire them to toggles in the [menu](menu.md):

```cpp
chili::Menu::toggle("MUSIC", save.data.music, [] { chili::audio::set_music(save.data.music); save.store(); }),
```

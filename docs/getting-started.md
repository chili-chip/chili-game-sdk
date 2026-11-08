# Get started

This page adds the game SDK to a 32blit game that already builds.

!!! tip "Starting a new game?"
    [game-template](https://github.com/chili-chip/game-template) (and `chilichip new`, which uses it) already includes the SDK, with a title menu, a pause menu and a save file. Click **Use this template**, follow the [32blit SDK's getting started guide](https://sdk.chilichip.eu/getting-started/) to build it, and skip to [step 3](#3-use-a-module).

## 1. Add it in CMake

Open your game's `CMakeLists.txt`. After `find_package(32BLIT ...)`, fetch the SDK. After `blit_executable(...)`, link it.

```cmake hl_lines="3-9 13"
find_package (32BLIT CONFIG REQUIRED PATHS ../32blit-sdk $ENV{PATH_32BLIT_SDK})

include(FetchContent)
FetchContent_Declare(
  chili-game-sdk
  GIT_REPOSITORY https://github.com/chili-chip/chili-game-sdk.git
  GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(chili-game-sdk)

blit_executable (${PROJECT_NAME} ${PROJECT_SOURCE})

target_link_libraries(${PROJECT_NAME} chili-game-sdk)
```

!!! note "Order matters"
    `FetchContent_MakeAvailable` must come after `find_package(32BLIT ...)`: the SDK builds against the 32blit library your game already uses.

`GIT_TAG` pins a release, so a new SDK version never changes your game until you bump it. The releases are on the [releases page](https://github.com/chili-chip/chili-game-sdk/releases).

## 2. Build as usual

Nothing else changes. CMake downloads the SDK on the first configure.

=== "Desktop"

    ```sh
    cmake --preset linux
    cmake --build out/build/linux
    ```

=== "Console (.blit)"

    ```sh
    cmake --preset vgc-blit
    cmake --build out/build/vgc-blit
    ```

=== "Web"

    ```sh
    mkdir build.web && cd build.web
    emcmake cmake .. -D32BLIT_DIR=../../32blit-sdk
    make
    ```

## 3. Use a module

Include the header you need. Everything is in the `chili` namespace.

```cpp
#include "32blit.hpp"
#include "chili/input.hpp"

using namespace blit;

int choice = 0;

void update(uint32_t time) {
  chili::menu_nav(choice, 3);  // up/down with wrap
  if(chili::just_pressed(Button::A)) { /* ... */ }
}
```

Then read the module pages:

| If you want... | Read |
| --- | --- |
| a title, options or pause menu | [Menu](menu.md) |
| to split the game into title, play and game-over screens | [Scenes](scenes.md) |
| to keep settings and high scores | [Save and high scores](save.md) |
| a name entry, help page or score table | [Screens](screens.md) |
| music and sound effects | [Music and sound](audio.md) |
| particles, score popups or screen shake | [Effects](effects.md) |
| D-pad auto-repeat | [Input](input.md) |

[Chili Catch](demo.md) uses all of them in one file, so it is a good place to copy from.

## Work on the SDK and your game together

To try local SDK changes in a game, point FetchContent at your checkout instead of GitHub:

```sh
cmake --preset linux -DFETCHCONTENT_SOURCE_DIR_CHILI-GAME-SDK=$HOME/src/chili-game-sdk
```

## Match your game's look

The menu and screens draw with `chili::theme`. Set it once in `init()`:

```cpp
#include "chili/ui.hpp"

void init() {
  chili::theme.text = Pen(236, 245, 220);
  chili::theme.highlight = Pen(70, 150, 80, 150);
  chili::theme.accent = Pen(140, 230, 90);
  chili::theme.border = Pen(90, 170, 90);
}
```

| Field | Used for | Default |
| --- | --- | --- |
| `text` | labels, selected rows | `Pen(240, 240, 236)` |
| `dim` | unselected rows, hints | `Pen(140, 140, 150)` |
| `accent` | values on the selected row, selected letter | `Pen(255, 140, 40)` |
| `highlight` | the bar behind the selected row | `Pen(198, 40, 40, 170)` |
| `panel` | panel fill | `Pen(16, 16, 24, 235)` |
| `border` | panel border | `Pen(198, 40, 40)` |
| `gold` | first place, new score blink | `Pen(255, 210, 60)` |
| `shadow` | text shadow | `Pen(0, 0, 0, 200)` |

For more than colours, override the draw hooks: see [Menu: restyle it](menu.md#restyle-it).

## Menu sounds

Menus and screens do not play sounds themselves. They tell you what happened through one hook, so all your UI sounds live in one place:

```cpp
#include "chili/audio.hpp"
#include "chili/ui.hpp"

void ui_sound(chili::UiSound s) {
  switch(s) {
    case chili::UiSound::move:   chili::audio::play_sfx(sfx_move); break;
    case chili::UiSound::change:
    case chili::UiSound::select: chili::audio::play_sfx(sfx_select); break;
    case chili::UiSound::back:   chili::audio::play_sfx(sfx_back); break;
  }
}

void init() {
  chili::ui_sound = ui_sound;
}
```

| `UiSound` | When |
| --- | --- |
| `move` | the cursor moved (menu up/down, name entry left/right) |
| `change` | a value changed (toggle, choice, letter) |
| `select` | A on an action, or the next letter in name entry |
| `back` | B, or a screen closed |

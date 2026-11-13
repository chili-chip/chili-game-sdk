# chili game SDK

[![Build](https://github.com/chili-chip/chili-game-sdk/actions/workflows/build.yml/badge.svg)](https://github.com/chili-chip/chili-game-sdk/actions/workflows/build.yml)
[![Docs](https://github.com/chili-chip/chili-game-sdk/actions/workflows/docs.yml/badge.svg)](https://github.com/chili-chip/chili-game-sdk/actions/workflows/docs.yml)

A small C++ library for chilichip console games: menus, scenes, saves, high scores, name entry, music, input helpers and screen effects. It sits on top of the [chilichip console SDK](https://sdk.chilichip.eu/) ([repo](https://github.com/chili-chip/32blit-sdk)) and works on the console, desktop and web.

**Docs:** <https://game-sdk.chilichip.eu/>

| Module | Header |
| --- | --- |
| Input: `just_pressed`, `menu_nav`, auto-repeat | `chili/input.hpp` |
| Menu with action, toggle and choice rows | `chili/menu.hpp` |
| Name entry, text screen, score table | `chili/screens.hpp` |
| Scenes with fades and an input guard | `chili/scene.hpp` |
| Versioned save | `chili/save.hpp` |
| High-score table | `chili/scores.hpp` |
| Music from note strings, sound effects | `chili/audio.hpp` |
| Particles, floating text, shake, text helpers | `chili/fx.hpp` |
| Text box: typewriter text, pages, face, choices | `chili/dialogue.hpp` |

**RPG parts** (`chili-game-sdk-rpg` target, `chili/rpg/world.hpp`): a turn-based RPG on a grid played from one JSON game file (`.rpg`): tile maps, movement, stats and levels, bump combat, enemy AI, inventory and equipment, dialogue, quests and saves. See [RPG parts](https://game-sdk.chilichip.eu/rpg/) and the demo RPG, Pepper Village (art: [Micro Roguelike by Kenney](https://kenney.nl/assets/micro-roguelike), CC0).

## Add it to a game

In your game's `CMakeLists.txt`, after `find_package(32BLIT ...)`:

```cmake
include(FetchContent)
FetchContent_Declare(
  chili-game-sdk
  GIT_REPOSITORY https://github.com/chili-chip/chili-game-sdk.git
  GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(chili-game-sdk)
```

and after `blit_executable(...)`:

```cmake
target_link_libraries(${PROJECT_NAME} chili-game-sdk)
```

For the RPG parts, link `chili-game-sdk-rpg` instead. Then `#include "chili/menu.hpp"` and so on. See [Get started](https://game-sdk.chilichip.eu/getting-started/).

## Work on it

Check out `32blit-sdk` next to this repo, `pip install 32blit`, then:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/examples/demo/chili-demo      # Chili Catch, the demo game
./build/examples/rpg/chili-rpg-demo   # Pepper Village, the demo RPG
python format/validate.py examples/rpg/assets/pepper.rpg  # check a game file (pip install jsonschema)
```

More in [Development](https://game-sdk.chilichip.eu/development/).

## Licence

MIT. See [LICENSE](LICENSE). The demo RPG's art is CC0 by Kenney (`examples/rpg/assets/KENNEY-LICENSE.txt`).

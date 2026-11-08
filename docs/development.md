# Development

How to work on the SDK itself.

## Layout

| Path | Holds |
| --- | --- |
| `include/chili/` | The public headers, one per module. |
| `src/` | The code. |
| `tests/` | Unit tests, one file per module, and a fake 32blit API. |
| `examples/demo/` | Chili Catch, the demo game. |
| `tools/` | The screenshot tool for these docs. |
| `docs/` | This site (MkDocs). |

## Build and test

Check out [32blit-sdk](https://github.com/chili-chip/32blit-sdk) next to this repo and install its tools (`pip install 32blit`). Then:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests build only on desktop, when this repo is the top-level project. Games that fetch the SDK never build them.

| Option | Default | Builds |
| --- | --- | --- |
| `CHILI_GAME_SDK_TESTS` | on for desktop | the unit tests and the screenshot tool |
| `CHILI_GAME_SDK_DEMO` | on | Chili Catch |

### How the tests work

Each test file is its own executable. `tests/check.hpp` is a tiny runner: `TEST(name)`, `CHECK(cond)` and `CHECK_EQ(a, b)`.

The tests link the real 32blit engine but not a firmware. `tests/fake_api.cpp` stands in for the firmware:

| Fake | For |
| --- | --- |
| `fake::hold(buttons)` | Holds buttons for the next frame and works out `pressed`, like the firmware. |
| audio channels | Tests check `blit::channels[n].frequency` and the ADSR phase. |
| in-memory files | `read_save`/`write_save` work; `fake::files()` shows what was written. |
| a 128x128 screen | For the screenshot tool. |

Logic is tested: input, menu navigation, save versioning, score ranking, the scene switcher, the note parser and the sequencer. Drawing is checked by eye, in the demo and the screenshots.

### Write the test first

Every change in behaviour starts with a failing test in `tests/test_<module>.cpp`, then the code. A bug fix starts with a test that shows the bug.

## Build for the console and the web

```sh
# console (.blit)
cmake -S . -B build.vgc \
  -D32BLIT_DIR=../32blit-sdk \
  -DPICO_SDK_PATH=../pico-sdk -DPICO_EXTRAS_PATH=../pico-extras \
  -DPICO_BOARD=chilichip_vgc -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_TOOLCHAIN_FILE=../32blit-sdk/pico2.toolchain \
  -DBLIT_EXECUTABLE_PICO_BLIT=1
cmake --build build.vgc

# web
emcmake cmake -S . -B build.web -D32BLIT_DIR=../32blit-sdk
cmake --build build.web
```

CI builds all three on every push: desktop with the tests, web, and the console `.blit`.

## Screenshots

The screenshots here come from the demo, played with scripted input on the fake API, so they are the same every time. After changing how something looks:

```sh
pip install pillow
python3 tools/screenshots.py
```

It writes `docs/assets/shots/*.png` at 3x. The script of button presses is in `tools/shots.cpp`.

## Docs

```sh
pip install -r docs/requirements.txt
mkdocs serve
```

CI builds the site with `mkdocs build --strict` and deploys `main` to [game-sdk.chilichip.eu](https://game-sdk.chilichip.eu/) on Cloudflare Pages.

## Releases

Games pin a tag, so release with a tag:

1. Update the version in `CMakeLists.txt` (`project(... VERSION x.y.z)`).
2. Merge to `main`.
3. Tag `vX.Y.Z` and publish a GitHub release with the changes.

Use semantic versions: a change that breaks a game's code is a new major version.

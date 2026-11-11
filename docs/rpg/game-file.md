# Game file

A game file (`.rpg`) is one JSON object. It holds the whole game. This page lists every field; the parts' own pages explain how each one plays.

```cpp
std::string err = world.load(text);  // "" when it loaded
```

## A small game file

```json
{
  "format": "chili-rpg",
  "version": 1,
  "title": "Pepper Village",
  "sprites": "sheet.png",
  "background": "#222323",
  "start": { "room": "village", "x": 6, "y": 5 },
  "player": { "sprite": 4 },
  "rules": { "step_ms": 110, "bag": 12, "spread": 1, "crit": 10 },

  "stats": [
    { "id": "hp", "label": "HP", "start": 12 },
    { "id": "max_hp", "label": "MAX", "start": 12 },
    { "id": "atk", "label": "ATK", "start": 2 },
    { "id": "def", "label": "DEF", "start": 0 }
  ],
  "levels": { "xp": [6, 16, 32], "gain": { "max_hp": 3, "atk": 1 } },

  "tiles": [
    { "key": ".", "sprite": 15 },
    { "key": "T", "sprite": 74, "solid": true },
    { "key": "~", "sprite": 124, "water": true }
  ],
  "rooms": [{
    "id": "village",
    "ground": ["TTTTTTTTTTTTTTTT", "T..............T", "..."],
    "actors": [{ "kind": "slime", "x": 7, "y": 3 }]
  }],
  "kinds": [{ "id": "slime", "sprite": 22, "brain": "chase", "stats": { "hp": 3, "atk": 1 }, "xp": 2 }]
}
```

The full demo file is [`examples/rpg/assets/pepper.rpg`](https://github.com/chili-chip/chili-game-sdk/blob/main/examples/rpg/assets/pepper.rpg).

## Ids

Rooms, kinds, items, dialogue, quests, songs, stats and slots have an `id`: lower case letters, digits, `-` and `_`. Other parts name them by id (`"kind": "slime"`). When the file loads, ids become small numbers, so the game never compares strings while it plays.

Flags and variables need no list: naming one in an [event](events.md) creates it.

## Top level

| Field | Type | Needed | Meaning |
| --- | --- | --- | --- |
| `format` | `"chili-rpg"` | yes | Marks the file. |
| `version` | `1` | yes | The format version. |
| `title` | text | yes | The game's name. |
| `author` | text | | Who made it. |
| `sprites` | text | | The sprite sheet's file name, for the player app. C++ games set `world.sheet`. |
| `background` | `"#rrggbb"` | | Clears the screen behind the map. Default black. |
| `start` | `{room, x, y}` | yes | Where a new game starts. |
| `player` | [look](#looks) | yes | The player's sprites. |
| `rules` | [rules](#rules) | | Step time, diagonals, bag size, damage spread and crits. |
| `stats` | list | yes | [Stats](stats.md); must have `hp` and `max_hp`. |
| `levels` | `{xp, gain}` | | [Levels](stats.md#xp-and-levels). |
| `tiles` | list | yes | [Tile types](maps.md#tiles). |
| `rooms` | list | yes | [Rooms](maps.md#rooms). |
| `kinds` | list | | [Kinds of actors](ai.md#kinds): people, enemies, doors. |
| `slots` | list of ids | | [Equipment slots](inventory.md#equipment), for example `["weapon", "armour"]`. |
| `items` | list | | [Item kinds](inventory.md#items). |
| `dialogue` | list | | [Dialogue](dialogue.md). |
| `quests` | list | | [Quests](quests.md). |
| `songs` | list | | [Songs](#songs). |
| `lose` | [event](events.md) | | Runs when the player dies. See [player death](combat.md#when-the-player-dies). |

## Rules

| Field | Default | Meaning |
| --- | --- | --- |
| `step_ms` | `120` | How long one step slides, in ms. Holding the D-pad walks one step per `step_ms`. |
| `diagonal` | `false` | Allow diagonal steps (two D-pad buttons at once), for the player and the brains. |
| `bag` | `16` | Bag slots, 1 to 32. |
| `spread` | `0` | Damage varies by up to this much either way. |
| `crit` | `0` | Percent chance of a hit doing double damage. |

## Looks

The player and every kind of actor use the same fields for their sprites.

| Field | Meaning |
| --- | --- |
| `sprite` | The sprite for every direction. |
| `sprites` | `{down, up, left, right}`: a sprite per direction, overriding `sprite`. |
| `frames` | Walk frames, 1 to 4. Frame *n* is the sprite number plus *n*. |
| `flip` | With no `left` sprite, draw `right` mirrored. Default `true`. |

Sprites are 8x8 cells of the sprite sheet, numbered left to right, top to bottom, from 0.

## Songs

Songs use the [note strings of the music module](../audio.md#songs). A room plays its `music` when you enter it, and the `music` action switches songs.

```json
"songs": [
  { "id": "village", "step_ms": 170,
    "lead": "E5 - G5 - A5 - - - G5 - E5 - D5 - - - ",
    "bass": "C3 - - - G2 - - - A2 - - - E2 - - - ",
    "drums": "" }
]
```

## Errors

`load()` returns `""`, or one line that says where the problem is:

| Error | Why |
| --- | --- |
| `line 12: expected ',' or '}'` | Not valid JSON. |
| `items[3].bonus.atk: not a whole number` | A wrong type. Numbers are whole numbers. |
| `rooms[0].actors[1].kind: no kind "blob"` | An id that is not in the file. |
| `rooms[0].ground[8]: rows must all be 16 wide` | A map row of the wrong length. |
| `rooms[0].ground[3]: no tile '?'` | A map character with no tile type. |
| `dialogue[0].lines[2].choices[0].goto: no line "bye"` | A `goto` to a line id that does not exist. |
| `stats: needs "hp" and "max_hp"` | The two fixed stats are missing. |

On an error the world stays empty, so check the result while you write a game.

## Check a file

[`format/schema.json`](https://github.com/chili-chip/chili-game-sdk/blob/main/format/schema.json) is a JSON Schema for the format. It catches typos in field names, which the loader skips. Editors such as VS Code use it for hints when the file names it:

```json
{ "$schema": "https://raw.githubusercontent.com/chili-chip/chili-game-sdk/main/format/schema.json", "format": "chili-rpg" }
```

Or check files from the command line:

```sh
pip install jsonschema
python format/validate.py examples/rpg/assets/pepper.rpg
```

```text
examples/rpg/assets/pepper.rpg: ok
```

The schema checks the shape; the loader checks that ids exist. CI runs both on the demo file.

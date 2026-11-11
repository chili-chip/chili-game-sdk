# Events

Events are how a game file says "when this happens, do that". Dialogue lines, choices, items, doors, triggers, deaths and quest stages all run events. They take the place of a scripting language, and a no-code maker can show one as a simple form.

An event is a list of **conditions** and a list of **actions**:

```json
{ "if": [ { "flag": "well-clean" }, { "has": "key" } ],
  "do": [ { "set": "door-open" }, { "sound": "door" }, { "say": "door-opened" } ],
  "else": [ { "say": "door-locked" } ] }
```

- All conditions in `if` must pass. An empty or missing `if` always passes.
- Then the actions in `do` run in order. When a condition fails, `else` runs instead.
- Where a field takes an event, a plain list of actions is short for `{"do": [...]}`:

```json
"use": [ { "heal": 6 } ]
```

## Flags and variables

Story state lives in flags and variables. There is no list of them: naming one creates it.

| | Values | Starts at | Limit |
| --- | --- | --- | --- |
| Flag | on or off | off | 256 |
| Variable | -32768 to 32767 | 0 | 64 |

Both go into the [save](save.md).

## Conditions

| Condition | Passes when | Example |
| --- | --- | --- |
| `flag` | the flag is on | `{ "flag": "met-elder" }` |
| `not` | the flag is off | `{ "not": "cave-seen" }` |
| `var` | the variable compares true to `value` (default `=`, value 0) | `{ "var": "gold", "op": ">=", "value": 10 }` |
| `has` | the player has at least `count` (default 1) of the item, equipped ones too | `{ "has": "potion", "count": 2 }` |
| `quest` with `stage` | the quest is at that stage; 0 is not started | `{ "quest": "stone", "stage": 2 }` |
| `quest` with `done` | the quest is done (or not, with `false`) | `{ "quest": "stone", "done": true } ` |
| `stat` | the player's stat (with equipment) compares true to `value` | `{ "stat": "atk", "op": ">", "value": 4 }` |

`op` is one of `=`, `!=`, `<`, `<=`, `>`, `>=`. It works with `var`, `stat` and `quest` stages:

```json
"if": [ { "quest": "stone", "stage": 2, "op": ">=" }, { "quest": "stone", "stage": 3, "op": "<=" } ]
```

## Actions

| Action | Does | Example |
| --- | --- | --- |
| `set` / `clear` | Turns a flag on / off | `{ "set": "door-open" }` |
| `add` | Adds `value` (can be negative) to a variable | `{ "add": "gold", "value": -5 }` |
| `give` / `take` | Puts `count` (default 1) items in the bag / takes them out | `{ "give": "sword" }` |
| `heal` | Heals the player, up to max hp | `{ "heal": 6 }` |
| `hurt` | Hurts the player | `{ "hurt": 2 }` |
| `stat` | Adds `value` to one of the player's base stats | `{ "stat": "atk", "value": 1 }` |
| `xp` | Gives xp (can level up) | `{ "xp": 20 }` |
| `warp` | Moves the player to a room and tile, with a fade | `{ "warp": "cave", "x": 2, "y": 2 }` |
| `say` | Starts a [dialogue](dialogue.md) | `{ "say": "elder" }` |
| `quest` with `stage` | Sets a [quest](quests.md)'s stage; stage 1 starts it | `{ "quest": "stone", "stage": 1 }` |
| `quest` with `done` | Finishes a quest | `{ "quest": "stone", "done": true }` |
| `sound` | Plays a built-in sound | `{ "sound": "door" }` |
| `music` | Plays a song from `songs` | `{ "music": "cave" }` |
| `save` | Saves the game | `{ "save": true }` |
| `end` | Ends the game, won or lost | `{ "end": "win" }` |
| `remove` | Removes the actor the event belongs to, for good (a door, a chest) | `{ "remove": true }` |
| `call` | Runs C++ the game registered under that name | `{ "call": "open_shop" }` |

Sounds: `step`, `bump`, `hit`, `hurt`, `miss`, `pickup`, `door`, `level`, `save`, `quest`, `win`, `lose`. A game can [replace them](hooks.md#sound).

`call` is the bridge to code. A game made only from a file has no C++, so an unknown name does nothing. See [hooks](hooks.md#actions).

## Where events run

| Field | Runs when |
| --- | --- |
| Item `use` | The player uses the item ([inventory](inventory.md)) |
| Kind or actor `bump` | The player walks into (or presses A at) a friendly actor |
| Kind `death` | An actor of that kind dies |
| Room trigger | The player steps on its tile ([maps](maps.md#triggers)) |
| Dialogue line `if` / `do` | The line shows only when `if` passes; `do` runs after it is read |
| Choice `do` | The player picks the choice |
| Quest stage `done` | The stage's goal is met |
| Top-level `lose` | The player dies |

## From C++

```cpp
world.run(event);              // check, then do or else
bool ok = world.check(event);  // only the conditions
world.set_flag(Game::find(world.game.flags, "door-open"), true);
int gold = world.var(Game::find(world.game.vars, "gold"));
```

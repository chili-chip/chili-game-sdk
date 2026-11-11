# Hooks

Hooks let C++ change a few rules the game file cannot. Every hook is optional. A game made only from a file (later, in the RPG maker) runs with none of them set, so a hook should add to a game, never be needed to finish it.

All hooks live in `world.hooks`.

| Hook | Called | Default without it |
| --- | --- | --- |
| [`damage`](#damage) | For each attack | `max(1, atk - def)`, spread, crits |
| [`on_level_up`](#on_level_up) | After each level up | nothing |
| [`on_use_item`](#on_use_item) | Before an item is used | the item's `use` event |
| [`actions`](#actions) | By the `call` action | nothing |
| [`brains`](#brains) | Each turn, for kinds with that brain name | the actor stands still |
| [`draw_tile`](#draw_tile) | For each tile drawn | the tile's sprite |
| [`sound`](#sound) | For each built-in sound | the SDK's sound effects |

## damage

```cpp
world.hooks.damage = [](chili::rpg::World &w, const chili::rpg::Actor &attacker, const chili::rpg::Actor &defender) {
  return std::max(0, w.stat(attacker, "atk") * 2 - w.stat(defender, "def"));
};
```

Return the damage. 0 is a miss. See [combat](combat.md#your-own-formula).

## on_level_up

```cpp
world.hooks.on_level_up = [](chili::rpg::World &w) {
  if(w.state.level == 5) w.give(chili::rpg::Game::find(w.game.items, "crown"));
};
```

## on_use_item

Return true when you handled the item; its `use` event then does not run and nothing is used up. See [inventory](inventory.md#on_use_item).

## actions

The game file runs these with `{ "call": "name" }`:

```cpp
world.hooks.actions["open_shop"] = [](chili::rpg::World &w) { shop.open(); };
```

```json
{ "id": "trader", "sprite": 9, "friendly": true, "bump": [ { "call": "open_shop" } ] }
```

## brains

Custom AI, by the name a kind gives as its `brain`. See [enemy AI](ai.md#your-own-brain).

```cpp
world.hooks.brains["hop"] = [](chili::rpg::World &w, chili::rpg::Actor &a) { w.step(a, 0, 1); };
```

## draw_tile

```cpp
world.hooks.draw_tile = [](chili::rpg::World &w, uint8_t tile, blit::Point at) { /* ... */ };
```

See [maps](maps.md#drawing-tiles-your-way).

## sound

```cpp
world.hooks.sound = [](chili::rpg::Sound s) {
  if(s == chili::rpg::Sound::hit) chili::audio::play_sfx(my_hit);
};
```

Sounds: `step`, `bump`, `hit`, `hurt`, `miss`, `pickup`, `door`, `level`, `save`, `quest`, `win`, `lose`. The defaults play on channels 4 to 6 ([music and sound](../audio.md)).

## What C++ can call

Hooks and your game can use the same functions the events use:

| Area | Functions |
| --- | --- |
| Moving | `step`, `warp`, `actor_at`, `blocked`, `busy` |
| Fighting | `attack`, `hurt`, `heal`, `gain_xp`, `stat` |
| Bag | `give`, `take`, `count`, `use_item`, `equip`, `drop` |
| Story | `flag`, `set_flag`, `var`, `say`, `set_quest`, `finish_quest`, `check_quests`, `run`, `check` |
| Game | `load`, `new_game`, `save`, `load_save`, `has_save`, `end`, `result` |
| Screens | `open(World::Screen::inventory)`, `draw_hud`, `draw_inventory`, `draw_quests`, `popup` |

Everything the world knows is public: `world.game` (the loaded file), `world.state` (what is saved), `world.player` and `world.actors`.

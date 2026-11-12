# Save

```cpp
world.save();          // write
world.load_save();     // read; false when there is no save for this game file
world.has_save();
```

The RPG state is one plain struct, `chili::rpg::State`, written with [`chili::Save`](../save.md). It is under 2 KB.

| Saved | |
| --- | --- |
| Where | Room, tile and facing |
| The player | Stats (with hp now), xp, level |
| The bag | Items, counts, what is equipped |
| Story | Flags, variables, quest stages and their counts, turn number |
| Each room | Actors gone (dead or removed), floor items taken, one-time triggers fired |
| The floor | Items dropped by the player or beaten enemies |

Not saved: where living enemies stand and how hurt they are. They are back in place at full health when the room loads.

## Which save

The magic number of the save is a hash of the game file's text. So:

- A save never loads into another game.
- After you edit the game file, old saves no longer load (`load_save()` returns false and the game starts fresh). An edited file can move rooms and items around, and an old save would point at the wrong things.

`world.save_slot` picks the 32blit save slot (default 0). If your game also saves its own settings with `chili::Save`, give them another slot.

## When it saves

| When | How |
| --- | --- |
| The player steps on a tile with `"save": true` | Plays the `save` sound and shows "SAVED" |
| An event runs `{ "save": true }` | |
| Your game calls `world.save()` | A pause menu entry, or `blit::on_quit` |

To save when the player quits:

```cpp
void init() {
  // ...
  blit::on_quit = [] { world.save(); };
}
```

## A continue button

```cpp
chili::Menu title{{
  chili::Menu::action("CONTINUE", [] { if(world.load_save()) scenes.go(world); }),
  chili::Menu::action("NEW GAME", [] { world.new_game(); scenes.go(world); }),
}};
```

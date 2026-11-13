# Movement and turns

The game is turn-based on a grid. The player acts, then every other actor acts. Nothing moves while the game waits for input. ([Real-time mode](https://github.com/chili-chip/chili-game-sdk/issues/28) comes later as an option.)

## What a turn is

| The player does | Takes a turn? |
| --- | --- |
| Steps onto a free tile | yes |
| Walks into an enemy (attack) | yes |
| Presses B (wait) | yes |
| Walks into a wall or water | no |
| Walks into a friendly actor, or presses A at one (talk) | no |
| Opens the bag or the quest log | no |

After the player's turn, each other actor thinks once, in the room's order. Then [quest goals](quests.md) are checked.

## Smooth steps

A step moves the actor at once on the grid, and its sprite slides from the old tile over `step_ms` (from the file's [rules](game-file.md#rules)). A D-pad press steps at once. A press made while something still slides is kept and runs when the slide ends, so quick taps are never lost. Holding the D-pad waits `repeat_ms` and then keeps walking, one step per `step_ms`. So a slow tap is always one step.

| Rule | Default | |
| --- | --- | --- |
| `step_ms` | 80 | How long a step slides |
| `repeat_ms` | 250 | How long to hold before walking on |

Attacks show as a short lunge towards the target.

## Diagonals

With `"diagonal": true` in the rules, two D-pad buttons at once step diagonally, brains walk in 8 directions and "next to" includes corners. A diagonal step cannot cut past a wall corner.

## Facing and sprites

Every actor faces the way it last moved or attacked. It is drawn with the sprite for that direction ([looks](game-file.md#looks)). With no `left` sprite, `right` is drawn mirrored. With `frames` above 1, each step shows the next walk frame.

## Speed

A kind's `speed` makes it act every 1, 2 or 3 turns:

```json
{ "id": "villager", "sprite": 7, "friendly": true, "brain": "wander", "speed": 2 }
```

## From C++

```cpp
bool moved = world.step(actor, 1, 0);  // one tile right, if free
bool waiting = !world.busy();          // nothing sliding, no text or screen open
chili::rpg::Actor *a = world.actor_at(5, 3);
```

`step` checks walls, water and other actors, and runs the same things a step does in play: damage tiles, and for the player, items, triggers, save tiles and warps.

#pragma once

// A small game file the RPG tests share. Tests that need a variant replace a
// piece of it with edit().
//
// The room "home" (16x16):
//
//   y 0  ################
//   y 1  #@.....E.......#     @ player start (1,1), E elder (7,1)
//   y 2  #..............#
//   y 3  #....~~........#     ~ water (5,3) (6,3)
//   y 4  #..............#
//   y 5  #.....j...S....#     j jelly (6,5), S slime (10,5)
//   y 6  #..............#
//   y 7  #..........^...#     ^ spikes (11,7)
//   y 8  #..............>     > warp to the cave (15,8)
//   y 9  #....T.........#     T trigger (5,9)
//   y 10 #..............#
//   y 11 #..............#
//   y 12 #......D.......#     D door (7,12)
//   y 13 #......$.......#     $ save tile (7,13)
//   y 14 #..............#
//   y 15 ################

#include <string>

namespace fixture {

inline const char *game = R"({
  "format": "chili-rpg",
  "version": 1,
  "title": "Test",
  "sprites": "sheet.png",
  "background": "#222323",
  "start": { "room": "home", "x": 1, "y": 1 },
  "player": { "sprite": 4 },
  "rules": { "step_ms": 100, "bag": 4 },

  "stats": [
    { "id": "hp", "label": "HP", "start": 10 },
    { "id": "max_hp", "label": "MAX", "start": 10 },
    { "id": "atk", "label": "ATK", "start": 3 },
    { "id": "def", "label": "DEF", "start": 1 }
  ],
  "levels": { "xp": [5, 20], "gain": { "max_hp": 2, "atk": 1 } },

  "tiles": [
    { "key": ".", "sprite": 0 },
    { "key": "#", "sprite": 1, "solid": true },
    { "key": "~", "sprite": [2, 3], "water": true },
    { "key": "^", "sprite": 5, "damage": 2 },
    { "key": "$", "sprite": 6, "save": true },
    { "key": ">", "sprite": 7 }
  ],

  "rooms": [
    {
      "id": "home",
      "name": "Home",
      "ground": [
        "################",
        "#..............#",
        "#..............#",
        "#....~~........#",
        "#..............#",
        "#..............#",
        "#..............#",
        "#..........^...#",
        "#..............>",
        "#..............#",
        "#..............#",
        "#..............#",
        "#..............#",
        "#......$.......#",
        "#..............#",
        "################"
      ],
      "actors": [
        { "kind": "elder", "x": 7, "y": 1 },
        { "kind": "slime", "x": 10, "y": 5 },
        { "kind": "door", "x": 7, "y": 12 }
      ],
      "items": [ { "item": "jelly", "x": 6, "y": 5 } ],
      "warps": [ { "x": 15, "y": 8, "to": "cave", "tx": 1, "ty": 1 } ],
      "triggers": [ { "x": 5, "y": 9, "do": [ { "set": "stepped" }, { "add": "steps", "value": 1 } ] } ]
    },
    {
      "id": "cave",
      "ground": [
        "################################",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "#..............................#",
        "################################"
      ]
    }
  ],

  "kinds": [
    { "id": "elder", "name": "Elder", "sprite": 16, "friendly": true, "talk": "hello" },
    { "id": "slime", "name": "Slime", "sprite": 20, "brain": "still",
      "stats": { "hp": 4, "atk": 2 }, "xp": 3,
      "drops": [ { "item": "jelly", "chance": 100 } ] },
    { "id": "door", "name": "Door", "sprite": 30, "friendly": true,
      "bump": { "if": [ { "has": "key" } ], "do": [ { "take": "key" }, { "remove": true } ], "else": [ { "set": "locked" } ] } }
  ],

  "slots": [ "weapon", "charm" ],
  "items": [
    { "id": "jelly", "name": "Jelly", "text": "Heals 3.", "icon": 40, "stack": 3, "use": [ { "heal": 3 } ] },
    { "id": "stick", "name": "Stick", "icon": 41, "kind": "equip", "slot": "weapon", "bonus": { "atk": 2 } },
    { "id": "key", "name": "Key", "icon": 42, "kind": "key" }
  ],

  "dialogue": [
    { "id": "hello", "who": "elder", "lines": [
      { "text": "Hello there." },
      { "if": [ { "flag": "met" } ], "text": "Back again?" },
      { "text": "Bring me jelly?", "do": [ { "set": "met" } ], "choices": [
        { "text": "Yes", "do": [ { "quest": "jelly", "stage": 1 } ], "goto": "thanks" },
        { "text": "No" }
      ] },
      { "id": "thanks", "text": "Thank you!" }
    ] }
  ],

  "quests": [
    { "id": "jelly", "title": "Sticky", "stages": [
      { "log": "Bring 2 jellies.", "goal": { "collect": "jelly", "count": 2 }, "done": [ { "give": "stick" } ] },
      { "log": "Beat a slime.", "goal": { "defeat": "slime", "count": 1 } }
    ] }
  ],

  "songs": [ { "id": "tune", "step_ms": 120, "lead": "C4 E4 G4", "bass": "", "drums": "" } ]
})";

// The game file with `from` replaced by `to` (the first time it appears).
inline std::string edit(const std::string &from, const std::string &to) {
  std::string s = game;
  auto at = s.find(from);
  if(at != std::string::npos) s.replace(at, from.size(), to);
  return s;
}

}

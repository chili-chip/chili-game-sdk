// Runs the web player in Node: loads Pepper Village, walks, talks, restarts.
//   node players/rpg/web/test.mjs build-web
import { readFileSync } from "node:fs";
import { resolve } from "node:path";
import { pathToFileURL } from "node:url";
import assert from "node:assert/strict";

const dir = resolve(process.argv[2] || "build-web");
const { default: createChiliRpg } = await import(pathToFileURL(`${dir}/chili-rpg.js`));
const { ChiliRpgPlayer, BUTTONS } = await import(pathToFileURL(`${dir}/player.js`));
const village = readFileSync(new URL("../../../examples/rpg/assets/pepper-village.rpg", import.meta.url), "utf8");

const player = await ChiliRpgPlayer.create(createChiliRpg);

// Errors say what is wrong.
assert.match(player.load("{}"), /format/);
assert.match(player.load("{"), /line 1/);
assert.equal(player.debug().state, "none");

assert.equal(player.load(village), null);
let d = player.debug();
assert.equal(d.state, "playing");
assert.equal(d.title, "Pepper Village");
assert.equal(d.room, "village");
assert.deepEqual([d.x, d.y], [6, 5]);
assert.equal(d.hp, 12);
assert.equal(d.talking, false);

// Holds a button for one step, then lets the step slide finish.
function press(name, steps = 20) {
  player.step(BUTTONS[name]);
  for (let i = 0; i < steps; i++) player.step(0);
}

press("up");
press("up");
press("left");
assert.deepEqual([player.debug().x, player.debug().y], [5, 3]);

// Walk into Elder Ana: she talks.
press("left", 120);
d = player.debug();
assert.equal(d.talking, true);
assert.equal(d.speaker, "Elder Ana");
assert.match(d.text, /sun stone/);

// The screen is 128 x 128 RGBA, in Sweetie 16 and the background colour.
const sweetie = new Set(["1a1c2c", "5d275d", "b13e53", "ef7d57", "ffcd75", "a7f070", "38b764", "257179",
  "29366f", "3b5dc9", "41a6f6", "73eff7", "f4f4f4", "94b0c2", "566c86", "333c57"]);
const pixels = player.rgba();
assert.equal(pixels.length, 128 * 128 * 4);
const hex = (i) => [0, 1, 2].map((k) => pixels[i * 4 + k].toString(16).padStart(2, "0")).join("");
assert.ok(sweetie.has(hex(64 * 128 + 2)), `a tree at the left edge, got ${hex(64 * 128 + 2)}`);
assert.equal(pixels[3], 255);

// Talking plays a blip for each letter: the audio is not silent.
const samples = player.audio(2205);
assert.equal(samples.length, 2205);
assert.ok(samples.some((s) => s !== 0), "some sound");

// Restart: a new game at the start.
player.restart();
d = player.debug();
assert.deepEqual([d.room, d.x, d.y, d.talking], ["village", 6, 5, false]);

// A tap that ends before the next step still counts.
player.press(BUTTONS.down, true);
player.press(BUTTONS.down, false);
for (let i = 0; i < 20; i++) player.step();
assert.equal(player.debug().y, 6);

// onStep sees every step.
let steps = 0;
player.onStep = () => steps++;
player.step(0);
player.step(0);
assert.equal(steps, 2);

console.log("web player: ok");

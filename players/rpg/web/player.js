// Chili RPG web player: runs the wasm build, draws to a canvas, reads keys and
// plays sound. See https://game-sdk.chilichip.eu/rpg/players/
//
//   import createChiliRpg from "./chili-rpg.js";
//   import { ChiliRpgPlayer } from "./player.js";
//   const player = await ChiliRpgPlayer.create(createChiliRpg);
//   const error = player.load(text);
//   if (!error) player.start(canvas);

export const BUTTONS = { up: 1, down: 2, left: 4, right: 8, a: 16, b: 32, x: 64, y: 128 };

const KEYS = {
  ArrowUp: BUTTONS.up, KeyW: BUTTONS.up,
  ArrowDown: BUTTONS.down, KeyS: BUTTONS.down,
  ArrowLeft: BUTTONS.left, KeyA: BUTTONS.left,
  ArrowRight: BUTTONS.right, KeyD: BUTTONS.right,
  KeyZ: BUTTONS.a, Space: BUTTONS.a, Enter: BUTTONS.a,
  KeyX: BUTTONS.b,
  KeyC: BUTTONS.x, KeyI: BUTTONS.x,
  KeyV: BUTTONS.y, KeyQ: BUTTONS.y,
};

const SIZE = 128;
const STEP_MS = 1000 / 60;
const SAMPLE_RATE = 22050;

export class ChiliRpgPlayer {
  static async create(createChiliRpg, options = {}) {
    return new ChiliRpgPlayer(await createChiliRpg(options));
  }

  constructor(module) {
    this.module = module;
    this.api = {
      load: module.cwrap("rpg_load", "number", ["string"]),
      error: module.cwrap("rpg_error", "number", []),
      step: module.cwrap("rpg_step", null, ["number"]),
      restart: module.cwrap("rpg_restart", null, []),
      rgba: module.cwrap("rpg_rgba", "number", []),
      audio: module.cwrap("rpg_audio", "number", ["number"]),
      debug: module.cwrap("rpg_debug", "number", []),
    };
    this.buttons = 0;    // held now, from keys and press()
    this.tapped = 0;     // pressed since the last step, so a quick tap is not lost
    this.onStep = null;  // called with debug() after every step, if set
    this.muted = false;
    this.frame = null;
    this.sound = null;
  }

  // Loads .rpg text and starts a new game. Returns null, or an error message.
  load(text) {
    return this.api.load(text) ? null : this.module.UTF8ToString(this.api.error());
  }

  // Starts the game again from the beginning.
  restart() { this.api.restart(); }

  // One 1/60 s step with a button mask.
  step(buttons = this.buttons | this.tapped) {
    this.tapped = 0;
    this.api.step(buttons);
    if (this.onStep) this.onStep(this.debug());
  }

  // { state, title, room, x, y, level, xp, turn, stats, hp, talking, speaker,
  //   text, choices, flags, vars, quests, bag }
  debug() { return JSON.parse(this.module.UTF8ToString(this.api.debug())); }

  // The screen as 128 x 128 RGBA pixels.
  rgba() {
    const at = this.api.rgba();
    return new Uint8ClampedArray(this.module.HEAPU8.buffer, at, SIZE * SIZE * 4).slice();
  }

  // The next `count` sound samples (-1 to 1, 22050 Hz).
  audio(count) {
    const at = this.api.audio(count);
    return new Float32Array(this.module.HEAPU8.buffer, at, count).slice();
  }

  // Press or release a button from your own controls (for example touch).
  press(button, down) {
    this.buttons = down ? this.buttons | button : this.buttons & ~button;
    if (down) this.tapped |= button;
  }

  // Runs the game in a canvas until stop(). Keys are read from `keys`.
  start(canvas, { keys = window } = {}) {
    this.stop();
    canvas.width = SIZE;
    canvas.height = SIZE;
    canvas.style.imageRendering = "pixelated";
    const ctx = canvas.getContext("2d");
    const onKey = (event) => {
      const button = KEYS[event.code];
      if (!button) return;
      event.preventDefault();
      this.press(button, event.type === "keydown");
      this.#startSound();
    };
    keys.addEventListener("keydown", onKey);
    keys.addEventListener("keyup", onKey);
    const onBlur = () => { this.buttons = 0; };
    window.addEventListener("blur", onBlur);

    let last = performance.now();
    let behind = 0;
    const tick = (now) => {
      behind = Math.min(behind + now - last, 250);
      last = now;
      let ran = 0;
      while (behind >= STEP_MS) {
        this.step();
        behind -= STEP_MS;
        ran++;
      }
      ctx.putImageData(new ImageData(this.rgba(), SIZE, SIZE), 0, 0);
      if (this.sound && ran) this.sound.play(this.audio(Math.round((ran * STEP_MS * SAMPLE_RATE) / 1000)), this.muted);
      this.frame = requestAnimationFrame(tick);
    };
    this.frame = requestAnimationFrame(tick);
    this.cleanup = () => {
      keys.removeEventListener("keydown", onKey);
      keys.removeEventListener("keyup", onKey);
      window.removeEventListener("blur", onBlur);
    };
  }

  stop() {
    if (this.frame) cancelAnimationFrame(this.frame);
    this.frame = null;
    if (this.cleanup) this.cleanup();
    this.cleanup = null;
    this.buttons = 0;
  }

  /** Turns sound on now. Call it from a click or key press: browsers block sound before one. */
  enableSound() {
    this.#startSound();
  }

  #startSound() {
    if (!this.sound && typeof AudioContext !== "undefined") this.sound = new Sound();
    if (this.sound) this.sound.resume();
  }
}

// Plays the samples the game makes, one short buffer after another.
class Sound {
  constructor() {
    this.ctx = new AudioContext();
    this.gain = this.ctx.createGain();
    this.gain.gain.value = 0.5;
    this.gain.connect(this.ctx.destination);
    this.at = 0;
  }

  resume() {
    if (this.ctx.state === "suspended") this.ctx.resume();
  }

  play(samples, muted) {
    if (muted || this.ctx.state !== "running" || !samples.length) return;
    const buffer = this.ctx.createBuffer(1, samples.length, SAMPLE_RATE);
    buffer.copyToChannel(samples, 0);
    const source = this.ctx.createBufferSource();
    source.buffer = buffer;
    source.connect(this.gain);
    // Keep a little ahead of now; start over after a pause.
    const now = this.ctx.currentTime;
    if (this.at < now || this.at > now + 0.2) this.at = now + 0.05;
    source.start(this.at);
    this.at += buffer.duration;
  }
}

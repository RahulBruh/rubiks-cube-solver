// Cross-checks the browser's sticker model (web/cube.js) against the C++
// solver. Usage:
//   node tests/model.test.mjs                 JS-only checks
//   node tests/model.test.mjs build/cubesolver   also compare with the C++ CLI
//   node tests/model.test.mjs --wasm          also solve through web/solver.js
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { createRequire } from "node:module";
import * as cube from "../web/cube.js";

let checks = 0;
const ok = (cond, msg) => { assert.ok(cond, msg); checks++; };

// Deterministic PRNG so failures reproduce.
let seed = 12345;
const rand = (n) => ((seed = (seed * 1103515245 + 12345) >>> 0) >>> 8) % n;
const randomMoves = (len) => Array.from({ length: len }, () => rand(18));

ok(cube.applyMove(cube.SOLVED, 3) === "UUFUUFUUFRRRRRRRRRFFDFFDFFDDDBDDBDDBLLLLLLLLLUBBUBBUBB", "R reference");
for (let m = 0; m < 18; m++) {
  ok(new Set(cube.PERM[m]).size === 54, `move ${m} is a permutation`);
  ok(cube.applyMoves(cube.SOLVED, [m, cube.inverseMove(m)]) === cube.SOLVED, `move ${m} inverse`);
  ok(cube.applyMoves(cube.SOLVED, [m, m, m, m]) === cube.SOLVED, `move ${m} order 4`);
}
const sexy = cube.parseMoves("R U R' U'");
ok(cube.applyMoves(cube.SOLVED, Array(6).fill(sexy).flat()) === cube.SOLVED, "(R U R' U')^6");
ok(cube.formatMoves(cube.parseMoves("r u2 F’ D3 b2'")) === "R U2 F' D' B2", "parse");
assert.throws(() => cube.parseMoves("R X"));
for (let i = 0; i < 50; i++) {
  const ms = randomMoves(25);
  ok(cube.applyMoves(cube.applyMoves(cube.SOLVED, ms), cube.invertMoves(ms)) === cube.SOLVED, "invert");
}

const cli = process.argv.find((a) => a.includes("cubesolver"));
if (cli) {
  for (let i = 0; i < 200; i++) {
    const ms = randomMoves(1 + rand(30));
    const fromCpp = execFileSync(cli, ["apply", cube.formatMoves(ms)]).toString().trim();
    ok(fromCpp === cube.applyMoves(cube.SOLVED, ms), `C++ and JS agree on ${cube.formatMoves(ms)}`);
  }
  console.log("C++ CLI agrees with the JS model on 200 random sequences");
}

if (process.argv.includes("--wasm")) {
  const require = createRequire(import.meta.url);
  const M = await require("../web/solver.js")();
  console.log(`wasm tables built in ${M._cs_init().toFixed(0)} ms`);
  const solve = M.cwrap("cs_solve", "string", ["string", "number", "number", "number"]);
  const randomState = M.cwrap("cs_random_state", "string", ["number"]);
  let total = 0;
  for (let i = 0; i < 100; i++) {
    const state = i % 2 ? randomState(i) : cube.applyMoves(cube.SOLVED, randomMoves(40));
    const r = JSON.parse(solve(state, 30, 21, 1000));
    ok(r.ok, r.error);
    ok(cube.isSolved(cube.applyMoves(state, cube.parseMoves(r.solution))), "wasm solution solves the JS cube");
    total += r.length;
  }
  const bad = JSON.parse(solve(cube.SOLVED.replace("U", "R"), 30, 21, 1000));
  ok(!bad.ok && /9 stickers/.test(bad.error), "wasm reports invalid input");
  console.log(`wasm solved 100 cubes, avg ${(total / 100).toFixed(2)} moves`);
}

console.log(`${checks} checks passed`);

# Rubik's Cube Solver

A Rubik's Cube solver in C++17 using Herbert Kociemba's two-phase algorithm. It solves any position in about 20 moves, usually in tens of milliseconds. There is no external dependency, just the standard library.

## How it works

The solver runs in two phases, each an IDA* search guided by pruning tables:

1. **Phase 1** moves the cube into the subgroup G1 = ⟨U, D, R2, L2, F2, B2⟩. In G1 every piece is oriented and the four middle-slice edges sit in the middle slice. Phase 1 tracks three coordinates: corner twist (3⁷ = 2,187), edge flip (2¹¹ = 2,048), and slice-edge placement (C(12,4) = 495).
2. **Phase 2** finishes the cube using only the 10 moves that stay inside G1. It tracks corner permutation (8! = 40,320), U/D-edge permutation (8!) and slice-edge permutation (4! = 24).

Each phase has two pruning tables, filled by breadth-first search with the exact distance to the goal for a pair of coordinates. The larger of the two values is an admissible heuristic. The four tables hold about 4 M entries and take roughly 270 ms to build at startup.

The search does not stop at the first answer. It keeps lengthening phase 1 and accepts only strictly shorter totals. It returns when it reaches the target length (20 by default) or when the time budget runs out. Before the two-phase search, a plain IDA* over all 18 moves looks for solutions of up to 7 moves (`--exact N` changes this), so turning `R U R' U'` gives back `U R U' R'` and not an 11-move two-phase answer. The phase-1 bound also bounds the whole solve, so on a well-scrambled cube this pre-search stops at the root and costs nothing. Its worst case is about 90 ms, for positions that are already close to G1.

```
src/cubie.*    cube model: piece permutation/orientation, facelet I/O, validation
src/coords.*   coordinates, move tables, pruning tables
src/solver.*   exact short search + two-phase IDA*
src/main.cpp   cubesolver CLI
tests/         C++ unit tests (no framework)
```

## Build and run

```sh
make            # builds build/cubesolver and build/test_solver
make test       # runs the tests
make bench      # 500 random-state cubes
```

```sh
$ ./build/cubesolver solve-moves "R U R' U' F2 D L'"
7 moves (phase 1: 0) in 0.0 ms, 49 nodes (optimal)
L D' F2 U R U' R'

$ ./build/cubesolver solve RFUDURLRLUFFDRFFLBDUFLFBFDLRFDLDBBBLBRBBLDUUDRUDLBRUUR
20 moves (phase 1: 12) in 62.0 ms, 350035 nodes
F U' D R2 F2 L D F' B2 U L' B' L2 U L2 F2 R2 B2 D' R2

$ ./build/cubesolver scramble --seed 42     # random-state scramble
$ ./build/cubesolver apply "R U2 F'"        # facelets after a sequence
```

Facelet strings follow Kociemba's layout: 54 characters, faces in the order U R F D L B, each face read left to right and top to bottom. Any six sticker characters work, because the colors are taken from the centers. Invalid cubes are rejected with a specific reason: wrong sticker counts, impossible pieces, a twisted corner, a flipped edge, or a parity error.

## Results

These come from `cubesolver bench 500 --seed 1`, one run on an Intel i7-9750H under WSL2 (g++ 11, `-O2`). The cubes are uniformly random states, not random move sequences.

| Stop at | Avg length | Max | Median time | p90 time | Avg time |
|---|---|---|---|---|---|
| ≤ 22 moves | 21.65 | 22 | 3.3 ms | 12.5 ms | 5.1 ms |
| ≤ 20 moves (1 s budget, default) | 19.89 | 21 | 23.3 ms | 800 ms | 168 ms |
| best in 100 ms | 19.97 | 22 | 100 ms | 100 ms | 100 ms |

With the default settings, 457 of 500 cubes (91%) were solved in 20 moves or fewer and none needed more than 21. Table build time was 260–330 ms. The tests solve 150 more random cubes and the superflip (20 moves) and check every solution by applying it.

## Tests

`make test` runs about 4 million checks. They cover:

- move definitions against a reference facelet string
- move inverses and order, and the identity (R U R' U')⁶ = 1
- facelet round trips for 300 random cubes
- rejection of every kind of invalid input
- coordinate rank/unrank
- move tables against cubie multiplication
- the known pruning-table depths (9 for twist×slice, 14 for corners×slice in phase 2)
- end-to-end solves

`node tests/model.test.mjs build/cubesolver` also checks the browser's sticker model against the C++ cubie model on 200 random sequences.

## License

MIT

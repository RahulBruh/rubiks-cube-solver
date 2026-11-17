// Coordinates, move tables and pruning tables for Kociemba's two-phase
// algorithm.
//
// Phase 1 takes any cube into the subgroup G1 = <U, D, R2, L2, F2, B2>,
// where every corner and edge is oriented and the four middle-slice edges
// are somewhere in the middle slice. It is tracked by three coordinates:
//   twist (corner orientation, 3^7 = 2187), flip (edge orientation,
//   2^11 = 2048) and slice (which 4 of the 12 positions hold the slice
//   edges, C(12,4) = 495).
// Phase 2 solves the cube inside G1 using only the 10 G1 moves, tracked by
//   corner permutation (8! = 40320), permutation of the 8 U/D-layer edges
//   (8! = 40320) and permutation of the 4 slice edges (4! = 24).
//
// Each pruning table stores the exact distance-to-goal of a pair of
// coordinates, found by breadth-first search. The max of a phase's two
// tables is an admissible IDA* heuristic.
#pragma once

#include <cstdint>
#include <vector>

#include "cubie.hpp"

namespace cube::coords {

constexpr int kTwist = 2187, kFlip = 2048, kSlice = 495;
constexpr int kCornerPerm = 40320, kEdge8Perm = 40320, kSlicePerm = 24;
constexpr int kPhase2Moves = 10;
// Global indices of the moves allowed in phase 2: U U2 U' R2 F2 D D2 D' L2 B2.
extern const int kP2Move[kPhase2Moves];
bool is_phase2_move(int move);

int twist(const CubieCube& c);
int flip(const CubieCube& c);
int slice(const CubieCube& c);
int corner_perm(const CubieCube& c);
int edge8_perm(const CubieCube& c);   // valid only inside G1
int slice_perm(const CubieCube& c);   // valid only inside G1

void set_twist(CubieCube& c, int v);
void set_flip(CubieCube& c, int v);
void set_slice(CubieCube& c, int v);
void set_corner_perm(CubieCube& c, int v);
void set_edge8_perm(CubieCube& c, int v);
void set_slice_perm(CubieCube& c, int v);

struct Tables {
  // Move tables: [coordinate][move]. Phase-2 tables are indexed by the
  // phase-2 move number (0..9); corner_perm uses all 18 moves.
  std::vector<uint16_t> twist_move, flip_move, slice_move, corner_move;
  std::vector<uint16_t> edge8_move, slice_perm_move;
  // Pruning tables (distance in moves).
  std::vector<int8_t> twist_slice_prun, flip_slice_prun;
  std::vector<int8_t> corner_slice_prun, edge8_slice_prun;
  double build_ms = 0;
};

// Builds the tables on first call (~100 ms native) and returns them.
const Tables& tables();

}  // namespace cube::coords

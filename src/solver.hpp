// Two-phase solver (Kociemba). Phase 1 is an IDA* search to G1; for every
// phase-1 solution it finds, an IDA* phase-2 search finishes the cube. The
// search keeps lengthening phase 1 and only accepts strictly shorter total
// solutions, so the answer improves until it reaches `target_length` or the
// time budget runs out.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "cubie.hpp"

namespace cube {

struct SolveOptions {
  int max_length = 30;     // never return anything longer than this
  int target_length = 20;  // stop as soon as a solution this short is found
  int timeout_ms = 1000;   // after this, return the best solution so far
  int exact_depth = 7;     // first search exhaustively for solutions this short
};

struct SolveResult {
  bool ok = false;
  std::string error;
  std::vector<int> moves;
  double ms = 0;               // search time, excluding table build
  uint64_t nodes = 0;          // IDA* nodes expanded, both phases
  int phase1_length = 0;       // moves of the returned solution spent in phase 1
  int solutions_found = 0;     // improving solutions seen along the way
  bool timed_out = false;
  bool optimal = false;        // proven shortest (found by the short exact search)
};

SolveResult solve(const CubieCube& cube, const SolveOptions& options = {});
SolveResult solve_facelets(const std::string& facelets, const SolveOptions& options = {});

}  // namespace cube

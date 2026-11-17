#include "solver.hpp"

#include <algorithm>
#include <chrono>

#include "coords.hpp"

namespace cube {

namespace {

using namespace coords;
using Clock = std::chrono::steady_clock;

// Two consecutive moves on the same face are never needed, and moves on
// opposite faces commute, so only one order (U before D, R before L, F
// before B) is searched.
inline bool redundant(int face, int last_face) {
  return last_face >= 0 && (face == last_face || face + 3 == last_face);
}

class Search {
 public:
  Search(const CubieCube& start, const SolveOptions& opt)
      : t_(tables()), start_(start), opt_(opt), best_len_(opt.max_length + 1) {
    deadline_ = Clock::now() + std::chrono::milliseconds(opt.timeout_ms);
  }

  SolveResult run() {
    auto t0 = Clock::now();
    const int tw = twist(start_), fl = flip(start_), sl = slice(start_);
    // Two-phase answers are short but not shortest, which looks silly when
    // someone turns R and gets eight moves back. So first run a plain IDA*
    // over all 18 moves up to exact_depth plies. h1 is also a lower bound for the
    // whole solve, so on a well-scrambled cube (h1 ~ 8-9) this exits at the
    // root, and any answer it finds is provably optimal.
    const int short_limit = std::min(opt_.exact_depth, opt_.max_length);
    for (int depth = h1(tw, fl, sl); depth <= short_limit && !found_ && !stop_; ++depth) {
      if (exact(start_, tw, fl, sl, 0, depth, -1)) {
        best_.assign(moves_, moves_ + depth);
        best_len_ = depth;
        found_ = 1;
        optimal_ = true;
      }
    }
    for (int depth = h1(tw, fl, sl); depth < best_len_ && !stop_ && !optimal_; ++depth)
      phase1(tw, fl, sl, 0, depth, -1);

    SolveResult r;
    r.ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    r.nodes = nodes_;
    r.solutions_found = found_;
    r.timed_out = timed_out_;
    r.optimal = optimal_;
    if (found_) {
      r.ok = true;
      r.moves = best_;
      r.phase1_length = best_p1_;
    } else {
      r.error = "No solution of " + std::to_string(opt_.max_length) + " moves or fewer found" +
                (timed_out_ ? " within " + std::to_string(opt_.timeout_ms) + " ms" : "");
    }
    return r;
  }

 private:
  bool exact(const CubieCube& c, int tw, int fl, int sl, int n, int togo, int last_face) {
    if (togo == 0) return c.is_solved();
    if (tick()) return false;
    for (int face = 0; face < 6; ++face) {
      if (redundant(face, last_face)) continue;
      for (int p = 0; p < 3; ++p) {
        const int m = face * 3 + p;
        const int ntw = t_.twist_move[tw * kNumMoves + m];
        const int nfl = t_.flip_move[fl * kNumMoves + m];
        const int nsl = t_.slice_move[sl * kNumMoves + m];
        if (h1(ntw, nfl, nsl) > togo - 1) continue;
        CubieCube d = c;
        d.multiply(move_cube(m));
        moves_[n] = m;
        if (exact(d, ntw, nfl, nsl, n + 1, togo - 1, face)) return true;
        if (stop_) return false;
      }
    }
    return false;
  }

  int h1(int tw, int fl, int sl) const {
    return std::max(t_.twist_slice_prun[sl * kTwist + tw], t_.flip_slice_prun[sl * kFlip + fl]);
  }
  int h2(int cp, int ep, int sp) const {
    return std::max(t_.corner_slice_prun[sp * kCornerPerm + cp],
                    t_.edge8_slice_prun[sp * kEdge8Perm + ep]);
  }

  bool tick() {
    if ((++nodes_ & 0xFFF) == 0 && Clock::now() > deadline_) {
      timed_out_ = true;
      stop_ = true;
    }
    return stop_;
  }

  void phase1(int tw, int fl, int sl, int n, int togo, int last_face) {
    if (tick()) return;
    if (togo == 0) {
      // A phase-1 solution ending in a G1 move is a shorter one in disguise.
      if (tw == 0 && fl == 0 && sl == 0 && (n == 0 || !is_phase2_move(moves_[n - 1])))
        start_phase2(n);
      return;
    }
    for (int face = 0; face < 6; ++face) {
      if (redundant(face, last_face)) continue;
      for (int p = 0; p < 3; ++p) {
        const int m = face * 3 + p;
        const int ntw = t_.twist_move[tw * kNumMoves + m];
        const int nfl = t_.flip_move[fl * kNumMoves + m];
        const int nsl = t_.slice_move[sl * kNumMoves + m];
        if (h1(ntw, nfl, nsl) > togo - 1) continue;
        moves_[n] = m;
        phase1(ntw, nfl, nsl, n + 1, togo - 1, face);
        if (stop_) return;
      }
    }
  }

  void start_phase2(int n1) {
    CubieCube c = start_;
    for (int i = 0; i < n1; ++i) c.multiply(move_cube(moves_[i]));
    const int cp = corner_perm(c), ep = edge8_perm(c), sp = slice_perm(c);
    const int last_face = n1 ? move_face(moves_[n1 - 1]) : -1;
    for (int d2 = h2(cp, ep, sp); n1 + d2 < best_len_; ++d2) {
      if (phase2(cp, ep, sp, n1, d2, last_face)) {
        best_len_ = n1 + d2;
        best_.assign(moves_, moves_ + best_len_);
        best_p1_ = n1;
        ++found_;
        if (best_len_ <= opt_.target_length) stop_ = true;
        return;
      }
      if (stop_) return;
    }
  }

  bool phase2(int cp, int ep, int sp, int n, int togo, int last_face) {
    if (togo == 0) return cp == 0 && ep == 0 && sp == 0;
    if (tick()) return false;
    for (int k = 0; k < kPhase2Moves; ++k) {
      const int m = kP2Move[k];
      if (redundant(move_face(m), last_face)) continue;
      const int ncp = t_.corner_move[cp * kNumMoves + m];
      const int nep = t_.edge8_move[ep * kPhase2Moves + k];
      const int nsp = t_.slice_perm_move[sp * kPhase2Moves + k];
      if (h2(ncp, nep, nsp) > togo - 1) continue;
      moves_[n] = m;
      if (phase2(ncp, nep, nsp, n + 1, togo - 1, move_face(m))) return true;
      if (stop_) return false;
    }
    return false;
  }

  const Tables& t_;
  CubieCube start_;
  SolveOptions opt_;
  Clock::time_point deadline_;
  int moves_[64] = {};
  int best_len_;
  std::vector<int> best_;
  int best_p1_ = 0;
  int found_ = 0;
  uint64_t nodes_ = 0;
  bool stop_ = false;
  bool timed_out_ = false;
  bool optimal_ = false;
};

}  // namespace

SolveResult solve(const CubieCube& cube, const SolveOptions& options) {
  std::string err = verify(cube);
  if (!err.empty()) {
    SolveResult r;
    r.error = err;
    return r;
  }
  SolveOptions opt = options;
  opt.max_length = std::clamp(opt.max_length, 0, 40);
  tables();  // build outside the timed search
  return Search(cube, opt).run();
}

SolveResult solve_facelets(const std::string& facelets, const SolveOptions& options) {
  CubieCube c;
  std::string err = from_facelets(facelets, c);
  if (!err.empty()) {
    SolveResult r;
    r.error = err;
    return r;
  }
  return solve(c, options);
}

}  // namespace cube

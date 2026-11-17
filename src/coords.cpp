#include "coords.hpp"

#include <chrono>

namespace cube::coords {

const int kP2Move[kPhase2Moves] = {0, 1, 2, 4, 7, 9, 10, 11, 13, 16};

bool is_phase2_move(int move) {
  for (int m : kP2Move)
    if (m == move) return true;
  return false;
}

namespace {

int choose(int n, int k) {
  if (k < 0 || k > n) return 0;
  int r = 1;
  for (int i = 0; i < k; ++i) r = r * (n - i) / (i + 1);
  return r;
}

// Lehmer-code rank of a permutation of 0..n-1 (values given relative).
int perm_rank(const uint8_t* p, int n) {
  int r = 0;
  for (int i = 0; i < n; ++i) {
    int smaller = 0;
    for (int j = i + 1; j < n; ++j)
      if (p[j] < p[i]) ++smaller;
    r = r * (n - i) + smaller;
  }
  return r;
}

void perm_unrank(int r, uint8_t* p, int n) {
  int digit[12];
  for (int i = n - 1; i >= 0; --i) {
    digit[i] = r % (n - i);
    r /= (n - i);
  }
  uint8_t avail[12];
  for (int i = 0; i < n; ++i) avail[i] = static_cast<uint8_t>(i);
  int left = n;
  for (int i = 0; i < n; ++i) {
    p[i] = avail[digit[i]];
    for (int k = digit[i]; k < left - 1; ++k) avail[k] = avail[k + 1];
    --left;
  }
}

// slice coordinate -> 12-bit mask of positions holding slice edges.
const std::vector<uint16_t>& slice_masks() {
  static const std::vector<uint16_t> masks = [] {
    std::vector<uint16_t> m(kSlice);
    for (int mask = 0; mask < 4096; ++mask) {
      if (__builtin_popcount(mask) != 4) continue;
      int a = 0, x = 0;
      for (int j = 11; j >= 0; --j)
        if (mask >> j & 1) a += choose(11 - j, ++x);
      m[a] = static_cast<uint16_t>(mask);
    }
    return m;
  }();
  return masks;
}

}  // namespace

int twist(const CubieCube& c) {
  int t = 0;
  for (int i = 0; i < 7; ++i) t = t * 3 + c.co[i];
  return t;
}

int flip(const CubieCube& c) {
  int t = 0;
  for (int i = 0; i < 11; ++i) t = t * 2 + c.eo[i];
  return t;
}

int slice(const CubieCube& c) {
  int a = 0, x = 0;
  for (int j = 11; j >= 0; --j)
    if (c.ep[j] >= FR) a += choose(11 - j, ++x);
  return a;
}

int corner_perm(const CubieCube& c) { return perm_rank(c.cp.data(), 8); }
int edge8_perm(const CubieCube& c) { return perm_rank(c.ep.data(), 8); }

int slice_perm(const CubieCube& c) {
  uint8_t p[4];
  for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(c.ep[8 + i] - 8);
  return perm_rank(p, 4);
}

void set_twist(CubieCube& c, int v) {
  int sum = 0;
  for (int i = 6; i >= 0; --i) {
    c.co[i] = static_cast<uint8_t>(v % 3);
    sum += c.co[i];
    v /= 3;
  }
  c.co[7] = static_cast<uint8_t>((3 - sum % 3) % 3);
}

void set_flip(CubieCube& c, int v) {
  int sum = 0;
  for (int i = 10; i >= 0; --i) {
    c.eo[i] = static_cast<uint8_t>(v & 1);
    sum += c.eo[i];
    v >>= 1;
  }
  c.eo[11] = static_cast<uint8_t>(sum & 1);
}

void set_slice(CubieCube& c, int v) {
  int mask = slice_masks()[v];
  uint8_t next_slice = FR, next_other = UR;
  for (int j = 0; j < 12; ++j)
    c.ep[j] = (mask >> j & 1) ? next_slice++ : next_other++;
}

void set_corner_perm(CubieCube& c, int v) { perm_unrank(v, c.cp.data(), 8); }
void set_edge8_perm(CubieCube& c, int v) { perm_unrank(v, c.ep.data(), 8); }

void set_slice_perm(CubieCube& c, int v) {
  uint8_t p[4];
  perm_unrank(v, p, 4);
  for (int i = 0; i < 4; ++i) c.ep[8 + i] = static_cast<uint8_t>(p[i] + 8);
}

namespace {

template <typename Get, typename Set>
std::vector<uint16_t> build_move_table(int size, const int* moves, int nmoves, Get get, Set set) {
  std::vector<uint16_t> t(static_cast<size_t>(size) * nmoves);
  for (int i = 0; i < size; ++i) {
    CubieCube c = CubieCube::solved();
    set(c, i);
    for (int k = 0; k < nmoves; ++k) {
      CubieCube d = c;
      d.multiply(move_cube(moves[k]));
      t[static_cast<size_t>(i) * nmoves + k] = static_cast<uint16_t>(get(d));
    }
  }
  return t;
}

// BFS over the product of a slice-type coordinate (n1 values, move table m1)
// and a second coordinate (n2 values, move table m2), both with `nmoves`
// columns. Entry index = a * n2 + b, goal = (0, 0).
std::vector<int8_t> build_prune(int n1, const std::vector<uint16_t>& m1, int stride1, int n2,
                                const std::vector<uint16_t>& m2, int stride2, const int* cols1,
                                const int* cols2, int nmoves) {
  const size_t total = static_cast<size_t>(n1) * n2;
  std::vector<int8_t> t(total, -1);
  t[0] = 0;
  size_t filled = 1;
  for (int depth = 0; filled < total; ++depth) {
    size_t added = 0;
    for (size_t i = 0; i < total; ++i) {
      if (t[i] != depth) continue;
      int a = static_cast<int>(i / n2), b = static_cast<int>(i % n2);
      for (int k = 0; k < nmoves; ++k) {
        size_t j = static_cast<size_t>(m1[static_cast<size_t>(a) * stride1 + cols1[k]]) * n2 +
                   m2[static_cast<size_t>(b) * stride2 + cols2[k]];
        if (t[j] < 0) {
          t[j] = static_cast<int8_t>(depth + 1);
          ++added;
        }
      }
    }
    if (!added) break;
    filled += added;
  }
  return t;
}

Tables build() {
  auto t0 = std::chrono::steady_clock::now();
  Tables t;
  int all[kNumMoves], p2local[kPhase2Moves];
  for (int i = 0; i < kNumMoves; ++i) all[i] = i;
  for (int i = 0; i < kPhase2Moves; ++i) p2local[i] = i;

  t.twist_move = build_move_table(kTwist, all, kNumMoves, twist, set_twist);
  t.flip_move = build_move_table(kFlip, all, kNumMoves, flip, set_flip);
  t.slice_move = build_move_table(kSlice, all, kNumMoves, slice, set_slice);
  t.corner_move = build_move_table(kCornerPerm, all, kNumMoves, corner_perm, set_corner_perm);
  t.edge8_move = build_move_table(kEdge8Perm, kP2Move, kPhase2Moves, edge8_perm, set_edge8_perm);
  t.slice_perm_move =
      build_move_table(kSlicePerm, kP2Move, kPhase2Moves, slice_perm, set_slice_perm);

  t.twist_slice_prun = build_prune(kSlice, t.slice_move, kNumMoves, kTwist, t.twist_move,
                                   kNumMoves, all, all, kNumMoves);
  t.flip_slice_prun = build_prune(kSlice, t.slice_move, kNumMoves, kFlip, t.flip_move,
                                  kNumMoves, all, all, kNumMoves);
  t.corner_slice_prun =
      build_prune(kSlicePerm, t.slice_perm_move, kPhase2Moves, kCornerPerm, t.corner_move,
                  kNumMoves, p2local, kP2Move, kPhase2Moves);
  t.edge8_slice_prun =
      build_prune(kSlicePerm, t.slice_perm_move, kPhase2Moves, kEdge8Perm, t.edge8_move,
                  kPhase2Moves, p2local, p2local, kPhase2Moves);

  t.build_ms =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  return t;
}

}  // namespace

const Tables& tables() {
  static const Tables t = build();
  return t;
}

}  // namespace cube::coords

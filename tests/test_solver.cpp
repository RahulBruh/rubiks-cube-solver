// Self-contained tests: no framework, exits non-zero on any failure.
#include <cstdio>
#include <string>
#include <vector>

#include "../src/coords.hpp"
#include "../src/solver.hpp"

using namespace cube;

static int failures = 0, checks = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    ++checks;                                                           \
    if (!(cond)) {                                                      \
      ++failures;                                                       \
      std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
    }                                                                   \
  } while (0)

static const std::string kSolved = "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB";

static CubieCube from_moves(const std::string& s) {
  std::vector<int> m;
  std::string err;
  parse_moves(s, m, err);
  CubieCube c = CubieCube::solved();
  c.apply_moves(m);
  return c;
}

static void test_moves() {
  std::puts("moves");
  CHECK(to_facelets(CubieCube::solved()) == kSolved);
  // Reference facelets after a single R turn.
  CHECK(to_facelets(from_moves("R")) == "UUFUUFUUFRRRRRRRRRFFDFFDFFDDDBDDBDDBLLLLLLLLLUBBUBBUBB");
  for (int m = 0; m < kNumMoves; ++m) {
    CubieCube c = CubieCube::solved();
    c.apply_move(m);
    CHECK(!c.is_solved());
    c.apply_move(inverse_move(m));
    CHECK(c.is_solved());
    CubieCube d = CubieCube::solved();
    for (int i = 0; i < 4; ++i) d.apply_move(m);
    CHECK(d.is_solved());
  }
  CHECK(from_moves("R U R' U' R U R' U' R U R' U' R U R' U' R U R' U' R U R' U'").is_solved());
  CHECK(!from_moves("R U R' U'").is_solved());
  // Superflip: every edge flipped in place, nothing else changed.
  CubieCube sf = from_moves("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2");
  for (int i = 0; i < 8; ++i) CHECK(sf.cp[i] == i && sf.co[i] == 0);
  for (int i = 0; i < 12; ++i) CHECK(sf.ep[i] == i && sf.eo[i] == 1);
}

static void test_parse() {
  std::puts("parse");
  std::vector<int> m;
  std::string err;
  CHECK(parse_moves("  R U2  F' D3 B2' ", m, err));
  CHECK(format_moves(m) == "R U2 F' D' B2");
  CHECK(!parse_moves("R X", m, err));
  CHECK(err.find("X") != std::string::npos);
  CHECK(format_moves(invert_moves({0, 4, 8})) == "F R2 U'");
}

static void test_facelets() {
  std::puts("facelets");
  for (uint32_t s = 0; s < 300; ++s) {
    CubieCube c = random_cube(s), back;
    CHECK(verify(c).empty());
    CHECK(from_facelets(to_facelets(c), back).empty());
    CHECK(back == c);
    CHECK(c.inverse().inverse() == c);
    CubieCube id = c;
    id.multiply(c.inverse());
    CHECK(id.is_solved());
  }
  // Arbitrary sticker letters work as long as the centers define them.
  std::string colors = to_facelets(from_moves("R U F"));
  for (char& ch : colors) ch = "wrgyob"[std::string("URFDLB").find(ch)];
  CubieCube c;
  CHECK(from_facelets(colors, c).empty());
  CHECK(c == from_moves("R U F"));
}

static void test_invalid() {
  std::puts("invalid input");
  CubieCube c;
  CHECK(from_facelets("UUU", c).find("54") != std::string::npos);
  std::string s = kSolved;
  s[0] = 'R';
  CHECK(from_facelets(s, c).find("9 stickers") != std::string::npos);
  s = kSolved;
  s[4] = 'R';
  CHECK(from_facelets(s, c).find("centers") != std::string::npos);

  CubieCube twisted = CubieCube::solved();
  twisted.co[0] = 1;
  twisted.co[1] = 0;
  CHECK(from_facelets(to_facelets(twisted), c).find("twisted") != std::string::npos);
  CubieCube flipped = CubieCube::solved();
  flipped.eo[0] = 1;
  CHECK(from_facelets(to_facelets(flipped), c).find("flipped") != std::string::npos);
  CubieCube swapped = CubieCube::solved();
  std::swap(swapped.ep[0], swapped.ep[1]);
  CHECK(from_facelets(to_facelets(swapped), c).find("swapped") != std::string::npos);
  // Two stickers of one edge recolored into an impossible pair (U/D colors).
  s = kSolved;
  s[5] = 'D';   // UR edge, U sticker
  s[28] = 'U';  // DF edge, D sticker
  CHECK(!from_facelets(s, c).empty());
  CHECK(!solve_facelets(s).ok);
}

static void test_coords() {
  std::puts("coordinates and tables");
  const auto& t = coords::tables();
  std::printf("  tables built in %.0f ms\n", t.build_ms);
  for (int v : {0, 1, 1000, 2186}) {
    CubieCube c = CubieCube::solved();
    coords::set_twist(c, v);
    CHECK(coords::twist(c) == v);
  }
  for (int v : {0, 7, 2047}) {
    CubieCube c = CubieCube::solved();
    coords::set_flip(c, v);
    CHECK(coords::flip(c) == v);
  }
  for (int v = 0; v < coords::kSlice; ++v) {
    CubieCube c = CubieCube::solved();
    coords::set_slice(c, v);
    CHECK(coords::slice(c) == v);
  }
  for (int v : {0, 1, 12345, 40319}) {
    CubieCube c = CubieCube::solved();
    coords::set_corner_perm(c, v);
    CHECK(coords::corner_perm(c) == v);
    coords::set_edge8_perm(c, v);
    CHECK(coords::edge8_perm(c) == v);
  }
  CHECK(coords::slice(CubieCube::solved()) == 0);
  // Move tables agree with cubie multiplication.
  for (uint32_t s = 0; s < 100; ++s) {
    CubieCube c = random_cube(s);
    for (int m = 0; m < kNumMoves; ++m) {
      CubieCube d = c;
      d.apply_move(m);
      CHECK(t.twist_move[coords::twist(c) * kNumMoves + m] == coords::twist(d));
      CHECK(t.flip_move[coords::flip(c) * kNumMoves + m] == coords::flip(d));
      CHECK(t.slice_move[coords::slice(c) * kNumMoves + m] == coords::slice(d));
      CHECK(t.corner_move[coords::corner_perm(c) * kNumMoves + m] == coords::corner_perm(d));
    }
  }
  // Pruning tables are complete and match known maximum depths.
  int max1 = 0, max2 = 0;
  for (int8_t v : t.twist_slice_prun) {
    CHECK(v >= 0);
    max1 = std::max<int>(max1, v);
  }
  for (int8_t v : t.corner_slice_prun) {
    CHECK(v >= 0);
    max2 = std::max<int>(max2, v);
  }
  for (int8_t v : t.flip_slice_prun) CHECK(v >= 0);
  for (int8_t v : t.edge8_slice_prun) CHECK(v >= 0);
  std::printf("  max depth: twist x slice %d, corners x slice %d\n", max1, max2);
  CHECK(max1 == 9);
  CHECK(max2 == 14);
}

static void test_solve() {
  std::puts("solve");
  SolveResult r = solve_facelets(kSolved);
  CHECK(r.ok && r.moves.empty());
  r = solve(from_moves("R"));
  CHECK(r.ok && format_moves(r.moves) == "R'");
  r = solve(from_moves("R U R' U'"));
  CHECK(r.ok && format_moves(r.moves) == "U R U' R'" && r.optimal);
  // Short scrambles come back no longer than the scramble, and solve it.
  for (const char* s : {"F2 L' D B R2 U'", "R U R' U' F2 D L'", "U R2 D F2 L2 U' B2"}) {
    CubieCube c = from_moves(s);
    std::vector<int> scramble;
    std::string err;
    parse_moves(s, scramble, err);
    r = solve(c);
    c.apply_moves(r.moves);
    CHECK(r.ok && r.optimal && r.moves.size() <= scramble.size() && c.is_solved());
  }

  int total = 0, worst = 0;
  const int n = 150;
  for (uint32_t s = 0; s < n; ++s) {
    CubieCube c = random_cube(1000 + s);
    r = solve(c);
    CHECK(r.ok);
    CubieCube d = c;
    d.apply_moves(r.moves);
    CHECK(d.is_solved());
    CHECK(static_cast<int>(r.moves.size()) <= 30);
    // No two moves on the same face in a row.
    for (size_t i = 1; i < r.moves.size(); ++i)
      CHECK(move_face(r.moves[i]) != move_face(r.moves[i - 1]));
    total += static_cast<int>(r.moves.size());
    worst = std::max(worst, static_cast<int>(r.moves.size()));
  }
  std::printf("  %d random cubes, avg %.2f moves, max %d\n", n, double(total) / n, worst);

  CubieCube sf = from_moves("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2");
  r = solve(sf, {30, 20, 5000});
  CubieCube d = sf;
  d.apply_moves(r.moves);
  CHECK(r.ok && d.is_solved());
  std::printf("  superflip: %zu moves in %.0f ms\n", r.moves.size(), r.ms);

  // max_length too small for a random cube within a tiny budget fails cleanly.
  r = solve(random_cube(7), {5, 5, 50});
  CHECK(!r.ok && !r.error.empty());
}

int main() {
  test_moves();
  test_parse();
  test_facelets();
  test_invalid();
  test_coords();
  test_solve();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}

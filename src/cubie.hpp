// Cubie-level model of the 3x3x3 cube: where each corner and edge piece sits
// and how it is twisted or flipped. Facelet strings use Kociemba's layout:
// 54 characters, faces in the order U R F D L B, each face read left to
// right, top to bottom as you look straight at it.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace cube {

enum Corner : uint8_t { URF, UFL, ULB, UBR, DFR, DLF, DBL, DRB };
enum Edge : uint8_t { UR, UF, UL, UB, DR, DF, DL, DB, FR, FL, BL, BR };

constexpr int kNumMoves = 18;  // 6 faces x {quarter, half, inverse}

struct CubieCube {
  std::array<uint8_t, 8> cp{};   // corner permutation: piece at each position
  std::array<uint8_t, 8> co{};   // corner orientation: 0, 1 or 2
  std::array<uint8_t, 12> ep{};  // edge permutation
  std::array<uint8_t, 12> eo{};  // edge orientation: 0 or 1

  static CubieCube solved();

  // Applies b after this cube (group product this * b).
  void multiply(const CubieCube& b);
  void apply_move(int move);
  void apply_moves(const std::vector<int>& moves);
  CubieCube inverse() const;
  bool is_solved() const;

  int corner_parity() const;
  int edge_parity() const;

  bool operator==(const CubieCube& o) const {
    return cp == o.cp && co == o.co && ep == o.ep && eo == o.eo;
  }
  bool operator!=(const CubieCube& o) const { return !(*this == o); }
};

// The cube that results from applying `move` (0..17) to a solved cube.
// move = face * 3 + power, faces U R F D L B, power 0 = quarter turn
// clockwise, 1 = half turn, 2 = counter-clockwise.
const CubieCube& move_cube(int move);

inline int move_face(int move) { return move / 3; }
int inverse_move(int move);

std::string move_name(int move);
std::string format_moves(const std::vector<int>& moves);
// Parses "R U2 F' ..." (whitespace separated). Returns false and fills
// `error` on an unknown token.
bool parse_moves(const std::string& text, std::vector<int>& out, std::string& error);
std::vector<int> invert_moves(const std::vector<int>& moves);

// Facelet conversion. Output always uses the letters U R F D L B.
std::string to_facelets(const CubieCube& c);

// Accepts any six distinct sticker characters: each face's color is taken
// from its center sticker. Returns an empty string on success, otherwise a
// human-readable reason the stickers can't be a real cube.
std::string from_facelets(const std::string& facelets, CubieCube& out);

// Checks the cubie invariants (each piece once, twist sum, flip sum, parity).
std::string verify(const CubieCube& c);

// A uniformly random reachable cube.
CubieCube random_cube(uint32_t seed);

}  // namespace cube

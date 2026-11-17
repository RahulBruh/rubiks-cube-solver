#include "cubie.hpp"

#include <algorithm>
#include <random>
#include <sstream>

namespace cube {

namespace {

// Face turns on a solved cube, in "replaced by" form: after the move,
// position i holds the piece that was at cp[i] (Kociemba's convention).
const CubieCube kBasic[6] = {
    // U
    {{UBR, URF, UFL, ULB, DFR, DLF, DBL, DRB},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {UB, UR, UF, UL, DR, DF, DL, DB, FR, FL, BL, BR},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    // R
    {{DFR, UFL, ULB, URF, DRB, DLF, DBL, UBR},
     {2, 0, 0, 1, 1, 0, 0, 2},
     {FR, UF, UL, UB, BR, DF, DL, DB, DR, FL, BL, UR},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    // F
    {{UFL, DLF, ULB, UBR, URF, DFR, DBL, DRB},
     {1, 2, 0, 0, 2, 1, 0, 0},
     {UR, FL, UL, UB, DR, FR, DL, DB, UF, DF, BL, BR},
     {0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0}},
    // D
    {{URF, UFL, ULB, UBR, DLF, DBL, DRB, DFR},
     {0, 0, 0, 0, 0, 0, 0, 0},
     {UR, UF, UL, UB, DF, DL, DB, DR, FR, FL, BL, BR},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    // L
    {{URF, ULB, DBL, UBR, DFR, UFL, DLF, DRB},
     {0, 1, 2, 0, 0, 2, 1, 0},
     {UR, UF, BL, UB, DR, DF, FL, DB, FR, UL, DL, BR},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    // B
    {{URF, UFL, UBR, DRB, DFR, DLF, ULB, DBL},
     {0, 0, 1, 2, 0, 0, 2, 1},
     {UR, UF, UL, BR, DR, DF, DL, BL, FR, FL, UB, DB},
     {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1}},
};

std::array<CubieCube, kNumMoves> build_moves() {
  std::array<CubieCube, kNumMoves> out;
  for (int f = 0; f < 6; ++f) {
    CubieCube c = CubieCube::solved();
    for (int p = 0; p < 3; ++p) {
      c.multiply(kBasic[f]);
      out[f * 3 + p] = c;
    }
  }
  return out;
}

const char kFaces[] = "URFDLB";

// Facelet indices of each corner's stickers, starting with its U/D sticker
// and going clockwise.
const int kCornerFacelet[8][3] = {
    {8, 9, 20},   // URF: U9 R1 F3
    {6, 18, 38},  // UFL: U7 F1 L3
    {0, 36, 47},  // ULB: U1 L1 B3
    {2, 45, 11},  // UBR: U3 B1 R3
    {29, 26, 15}, // DFR: D3 F9 R7
    {27, 44, 24}, // DLF: D1 L9 F7
    {33, 53, 42}, // DBL: D7 B9 L7
    {35, 17, 51}, // DRB: D9 R9 B7
};
const int kEdgeFacelet[12][2] = {
    {5, 10},  {7, 19},  {3, 37},  {1, 46},  // UR UF UL UB
    {32, 16}, {28, 25}, {30, 43}, {34, 52}, // DR DF DL DB
    {23, 12}, {21, 41}, {50, 39}, {48, 14}, // FR FL BL BR
};
// Face index (U=0 R=1 F=2 D=3 L=4 B=5) of each sticker of each solved piece.
const int kCornerColor[8][3] = {
    {0, 1, 2}, {0, 2, 4}, {0, 4, 5}, {0, 5, 1},
    {3, 2, 1}, {3, 4, 2}, {3, 5, 4}, {3, 1, 5},
};
const int kEdgeColor[12][2] = {
    {0, 1}, {0, 2}, {0, 4}, {0, 5}, {3, 1}, {3, 2},
    {3, 4}, {3, 5}, {2, 1}, {2, 4}, {5, 4}, {5, 1},
};

const char* kCornerNames[8] = {"URF", "UFL", "ULB", "UBR", "DFR", "DLF", "DBL", "DRB"};
const char* kEdgeNames[12] = {"UR", "UF", "UL", "UB", "DR", "DF",
                              "DL", "DB", "FR", "FL", "BL", "BR"};

template <size_t N>
int perm_parity(const std::array<uint8_t, N>& p) {
  int s = 0;
  for (size_t i = 0; i < N; ++i)
    for (size_t j = i + 1; j < N; ++j)
      if (p[i] > p[j]) ++s;
  return s & 1;
}

}  // namespace

CubieCube CubieCube::solved() {
  CubieCube c;
  for (int i = 0; i < 8; ++i) c.cp[i] = static_cast<uint8_t>(i);
  for (int i = 0; i < 12; ++i) c.ep[i] = static_cast<uint8_t>(i);
  return c;
}

void CubieCube::multiply(const CubieCube& b) {
  std::array<uint8_t, 8> ncp, nco;
  for (int i = 0; i < 8; ++i) {
    ncp[i] = cp[b.cp[i]];
    nco[i] = static_cast<uint8_t>((co[b.cp[i]] + b.co[i]) % 3);
  }
  std::array<uint8_t, 12> nep, neo;
  for (int i = 0; i < 12; ++i) {
    nep[i] = ep[b.ep[i]];
    neo[i] = static_cast<uint8_t>((eo[b.ep[i]] + b.eo[i]) & 1);
  }
  cp = ncp;
  co = nco;
  ep = nep;
  eo = neo;
}

const CubieCube& move_cube(int move) {
  static const std::array<CubieCube, kNumMoves> moves = build_moves();
  return moves[move];
}

void CubieCube::apply_move(int move) { multiply(move_cube(move)); }

void CubieCube::apply_moves(const std::vector<int>& moves) {
  for (int m : moves) apply_move(m);
}

CubieCube CubieCube::inverse() const {
  CubieCube r;
  for (int i = 0; i < 8; ++i) r.cp[cp[i]] = static_cast<uint8_t>(i);
  for (int i = 0; i < 8; ++i) r.co[i] = static_cast<uint8_t>((3 - co[r.cp[i]]) % 3);
  for (int i = 0; i < 12; ++i) r.ep[ep[i]] = static_cast<uint8_t>(i);
  for (int i = 0; i < 12; ++i) r.eo[i] = eo[r.ep[i]];
  return r;
}

bool CubieCube::is_solved() const { return *this == solved(); }
int CubieCube::corner_parity() const { return perm_parity(cp); }
int CubieCube::edge_parity() const { return perm_parity(ep); }

int inverse_move(int move) { return move - move % 3 + (2 - move % 3); }

std::string move_name(int move) {
  static const char* suffix[3] = {"", "2", "'"};
  return std::string(1, kFaces[move / 3]) + suffix[move % 3];
}

std::string format_moves(const std::vector<int>& moves) {
  std::string s;
  for (size_t i = 0; i < moves.size(); ++i) {
    if (i) s += ' ';
    s += move_name(moves[i]);
  }
  return s;
}

bool parse_moves(const std::string& text, std::vector<int>& out, std::string& error) {
  out.clear();
  std::istringstream in(text);
  std::string tok;
  while (in >> tok) {
    const char* f = nullptr;
    for (const char* p = kFaces; *p; ++p)
      if (*p == tok[0]) f = p;
    std::string rest = tok.substr(1);
    int power = -1;
    if (rest.empty()) power = 0;
    else if (rest == "2" || rest == "2'") power = 1;
    else if (rest == "'" || rest == "3") power = 2;
    if (!f || power < 0) {
      error = "Unknown move \"" + tok + "\"";
      return false;
    }
    out.push_back(static_cast<int>(f - kFaces) * 3 + power);
  }
  return true;
}

std::vector<int> invert_moves(const std::vector<int>& moves) {
  std::vector<int> r(moves.rbegin(), moves.rend());
  for (int& m : r) m = inverse_move(m);
  return r;
}

std::string to_facelets(const CubieCube& c) {
  std::string f(54, '?');
  for (int i = 0; i < 6; ++i) f[9 * i + 4] = kFaces[i];
  for (int i = 0; i < 8; ++i) {
    int piece = c.cp[i], ori = c.co[i];
    for (int n = 0; n < 3; ++n)
      f[kCornerFacelet[i][(n + ori) % 3]] = kFaces[kCornerColor[piece][n]];
  }
  for (int i = 0; i < 12; ++i) {
    int piece = c.ep[i], ori = c.eo[i];
    for (int n = 0; n < 2; ++n)
      f[kEdgeFacelet[i][(n + ori) % 2]] = kFaces[kEdgeColor[piece][n]];
  }
  return f;
}

std::string from_facelets(const std::string& facelets, CubieCube& out) {
  if (facelets.size() != 54)
    return "Expected 54 stickers, got " + std::to_string(facelets.size());

  // Map sticker characters to faces using the centers.
  int face_of[256];
  std::fill(face_of, face_of + 256, -1);
  for (int i = 0; i < 6; ++i) {
    unsigned char ch = static_cast<unsigned char>(facelets[9 * i + 4]);
    if (face_of[ch] != -1) return "Two centers have the same color";
    face_of[ch] = i;
  }
  int f[54], count[6] = {0, 0, 0, 0, 0, 0};
  for (int i = 0; i < 54; ++i) {
    int face = face_of[static_cast<unsigned char>(facelets[i])];
    if (face < 0)
      return std::string("Sticker color '") + facelets[i] + "' doesn't match any center";
    f[i] = face;
    ++count[face];
  }
  for (int i = 0; i < 6; ++i)
    if (count[i] != 9)
      return std::string("Each color needs exactly 9 stickers; the ") + kFaces[i] +
             "-center color has " + std::to_string(count[i]);

  CubieCube c;
  for (int i = 0; i < 8; ++i) {
    int ori = 0;
    while (ori < 3 && f[kCornerFacelet[i][ori]] != 0 && f[kCornerFacelet[i][ori]] != 3) ++ori;
    if (ori == 3) return std::string("The corner at ") + kCornerNames[i] + " has no U or D color";
    int c1 = f[kCornerFacelet[i][(ori + 1) % 3]];
    int c2 = f[kCornerFacelet[i][(ori + 2) % 3]];
    int piece = -1;
    for (int j = 0; j < 8; ++j)
      if (kCornerColor[j][0] == f[kCornerFacelet[i][ori]] && kCornerColor[j][1] == c1 &&
          kCornerColor[j][2] == c2)
        piece = j;
    if (piece < 0)
      return std::string("The corner at ") + kCornerNames[i] +
             " has a color combination that doesn't exist on a real cube";
    c.cp[i] = static_cast<uint8_t>(piece);
    c.co[i] = static_cast<uint8_t>(ori);
  }
  for (int i = 0; i < 12; ++i) {
    int a = f[kEdgeFacelet[i][0]], b = f[kEdgeFacelet[i][1]];
    int piece = -1, ori = 0;
    for (int j = 0; j < 12; ++j) {
      if (kEdgeColor[j][0] == a && kEdgeColor[j][1] == b) { piece = j; ori = 0; }
      if (kEdgeColor[j][0] == b && kEdgeColor[j][1] == a) { piece = j; ori = 1; }
    }
    if (piece < 0)
      return std::string("The edge at ") + kEdgeNames[i] +
             " has a color combination that doesn't exist on a real cube";
    c.ep[i] = static_cast<uint8_t>(piece);
    c.eo[i] = static_cast<uint8_t>(ori);
  }
  std::string err = verify(c);
  if (!err.empty()) return err;
  out = c;
  return "";
}

std::string verify(const CubieCube& c) {
  bool seen_c[8] = {}, seen_e[12] = {};
  for (int i = 0; i < 8; ++i) {
    if (c.cp[i] >= 8 || seen_c[c.cp[i]]) return "A corner piece appears twice";
    seen_c[c.cp[i]] = true;
  }
  for (int i = 0; i < 12; ++i) {
    if (c.ep[i] >= 12 || seen_e[c.ep[i]]) return "An edge piece appears twice";
    seen_e[c.ep[i]] = true;
  }
  int twist = 0, flip = 0;
  for (int i = 0; i < 8; ++i) twist += c.co[i];
  for (int i = 0; i < 12; ++i) flip += c.eo[i];
  if (twist % 3) return "A corner is twisted in place (corner twist doesn't add up)";
  if (flip % 2) return "An edge is flipped in place (edge flip doesn't add up)";
  if (c.corner_parity() != c.edge_parity())
    return "Two pieces are swapped (permutation parity error)";
  return "";
}

CubieCube random_cube(uint32_t seed) {
  std::mt19937 rng(seed);
  CubieCube c = CubieCube::solved();
  std::shuffle(c.cp.begin(), c.cp.end(), rng);
  std::shuffle(c.ep.begin(), c.ep.end(), rng);
  if (c.corner_parity() != c.edge_parity()) std::swap(c.ep[0], c.ep[1]);
  int twist = 0, flip = 0;
  for (int i = 0; i < 7; ++i) twist += c.co[i] = static_cast<uint8_t>(rng() % 3);
  for (int i = 0; i < 11; ++i) flip += c.eo[i] = static_cast<uint8_t>(rng() % 2);
  c.co[7] = static_cast<uint8_t>((3 - twist % 3) % 3);
  c.eo[11] = static_cast<uint8_t>(flip % 2);
  return c;
}

}  // namespace cube

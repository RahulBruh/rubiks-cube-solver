// C ABI exported to JavaScript by the Emscripten build. Results are returned
// as JSON in a static buffer that stays valid until the next call.
#include <cstdio>
#include <string>

#include "coords.hpp"
#include "solver.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

using namespace cube;

namespace {

std::string g_out;

std::string json_escape(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\';
    o += c;
  }
  return o;
}

}  // namespace

extern "C" {

// Builds the move and pruning tables; returns the time it took in ms.
EMSCRIPTEN_KEEPALIVE double cs_init() { return coords::tables().build_ms; }

EMSCRIPTEN_KEEPALIVE const char* cs_solve(const char* facelets, int max_length,
                                          int target_length, int timeout_ms) {
  SolveOptions opt;
  opt.max_length = max_length;
  opt.target_length = target_length;
  opt.timeout_ms = timeout_ms;
  SolveResult r = solve_facelets(facelets ? facelets : "", opt);
  char buf[160];
  if (!r.ok) {
    g_out = "{\"ok\":false,\"error\":\"" + json_escape(r.error) + "\"}";
  } else {
    std::snprintf(buf, sizeof buf,
                  ",\"length\":%zu,\"phase1\":%d,\"ms\":%.2f,\"nodes\":%llu,"
                  "\"improvements\":%d,\"timedOut\":%s,\"optimal\":%s}",
                  r.moves.size(), r.phase1_length, r.ms,
                  static_cast<unsigned long long>(r.nodes), r.solutions_found,
                  r.timed_out ? "true" : "false", r.optimal ? "true" : "false");
    g_out = "{\"ok\":true,\"solution\":\"" + format_moves(r.moves) + "\"" + buf;
  }
  return g_out.c_str();
}

// A uniformly random cube as a facelet string.
EMSCRIPTEN_KEEPALIVE const char* cs_random_state(unsigned seed) {
  g_out = to_facelets(random_cube(seed));
  return g_out.c_str();
}

}  // extern "C"

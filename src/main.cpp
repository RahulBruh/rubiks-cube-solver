// cubesolver: command-line front end for the two-phase solver.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "coords.hpp"
#include "solver.hpp"

using namespace cube;

namespace {

void usage() {
  std::puts(
      "usage:\n"
      "  cubesolver solve <54 facelets> [--target N] [--max N] [--timeout MS]\n"
      "  cubesolver apply \"<moves>\"          facelets after a move sequence\n"
      "  cubesolver solve-moves \"<moves>\"    solve the cube a scramble produces\n"
      "  cubesolver scramble [--seed S]      random-state scramble\n"
      "  cubesolver bench [N] [--seed S] [--target N] [--timeout MS]\n"
      "\n"
      "Facelets use Kociemba order: U1..U9 R1..R9 F1..F9 D1..D9 L1..L9 B1..B9.");
}

struct Args {
  std::vector<std::string> pos;
  SolveOptions opt;
  uint32_t seed = 1;
};

Args parse(int argc, char** argv, int from) {
  Args a;
  for (int i = from; i < argc; ++i) {
    auto next = [&](const char* name) {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "%s needs a value\n", name);
        std::exit(2);
      }
      return std::atoi(argv[++i]);
    };
    if (!std::strcmp(argv[i], "--target")) a.opt.target_length = next("--target");
    else if (!std::strcmp(argv[i], "--max")) a.opt.max_length = next("--max");
    else if (!std::strcmp(argv[i], "--timeout")) a.opt.timeout_ms = next("--timeout");
    else if (!std::strcmp(argv[i], "--exact")) a.opt.exact_depth = next("--exact");
    else if (!std::strcmp(argv[i], "--seed")) a.seed = static_cast<uint32_t>(next("--seed"));
    else a.pos.push_back(argv[i]);
  }
  return a;
}

int report(const SolveResult& r) {
  if (!r.ok) {
    std::fprintf(stderr, "error: %s\n", r.error.c_str());
    return 1;
  }
  std::printf("%s\n", format_moves(r.moves).c_str());
  std::fprintf(stderr, "%zu moves (phase 1: %d) in %.1f ms, %llu nodes%s\n", r.moves.size(),
               r.phase1_length, r.ms, static_cast<unsigned long long>(r.nodes),
               r.optimal ? " (optimal)" : r.timed_out ? " (time budget reached)" : "");
  return 0;
}

int bench(const Args& a) {
  int n = a.pos.empty() ? 200 : std::atoi(a.pos[0].c_str());
  std::fprintf(stderr, "building tables... ");
  std::fprintf(stderr, "%.0f ms\n", coords::tables().build_ms);
  std::vector<double> times;
  std::vector<int> lengths;
  int hist[41] = {};
  for (int i = 0; i < n; ++i) {
    CubieCube c = random_cube(a.seed + static_cast<uint32_t>(i));
    SolveResult r = solve(c, a.opt);
    CubieCube check = c;
    check.apply_moves(r.moves);
    if (!r.ok || !check.is_solved()) {
      std::fprintf(stderr, "cube %d: solver failed: %s\n", i, r.error.c_str());
      return 1;
    }
    times.push_back(r.ms);
    lengths.push_back(static_cast<int>(r.moves.size()));
    ++hist[r.moves.size()];
  }
  std::sort(times.begin(), times.end());
  double total = 0, len_total = 0;
  for (double t : times) total += t;
  for (int l : lengths) len_total += l;
  auto pct = [&](double p) { return times[std::min(times.size() - 1, size_t(p * times.size()))]; };
  std::printf("random-state cubes: %d  (target %d moves, budget %d ms)\n", n, a.opt.target_length,
              a.opt.timeout_ms);
  std::printf("avg length: %.2f moves   max: %d\n", len_total / n,
              *std::max_element(lengths.begin(), lengths.end()));
  std::printf("time  avg %.1f ms   p50 %.1f   p90 %.1f   p99 %.1f   max %.1f\n", total / n,
              pct(0.5), pct(0.9), pct(0.99), times.back());
  std::printf("length histogram:");
  for (int l = 0; l <= 40; ++l)
    if (hist[l]) std::printf("  %d:%d", l, hist[l]);
  std::printf("\n");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    usage();
    return 2;
  }
  std::string cmd = argv[1];
  Args a = parse(argc, argv, 2);

  if (cmd == "solve" && a.pos.size() == 1) return report(solve_facelets(a.pos[0], a.opt));

  if ((cmd == "apply" || cmd == "solve-moves") && a.pos.size() == 1) {
    std::vector<int> moves;
    std::string err;
    if (!parse_moves(a.pos[0], moves, err)) {
      std::fprintf(stderr, "error: %s\n", err.c_str());
      return 1;
    }
    CubieCube c = CubieCube::solved();
    c.apply_moves(moves);
    if (cmd == "apply") {
      std::printf("%s\n", to_facelets(c).c_str());
      return 0;
    }
    return report(solve(c, a.opt));
  }

  if (cmd == "scramble") {
    CubieCube c = random_cube(a.seed);
    SolveResult r = solve(c, a.opt);
    if (!r.ok) return report(r);
    std::printf("%s\n%s\n", to_facelets(c).c_str(), format_moves(invert_moves(r.moves)).c_str());
    return 0;
  }

  if (cmd == "bench") return bench(a);

  usage();
  return 2;
}

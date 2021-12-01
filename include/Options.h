#ifndef __HIERMUS_OPTIONS_H_
#define __HIERMUS_OPTIONS_H_

#include "Types.h"

#include <chrono>
#include <random>
#include <string>

namespace HierMUS {

class DriverOptions {
public:
  vector<string> input_files; // Support mzn and dzn files
  string fznpath;
  string pathpath;
  string oraclepath;

  int maxmuses = 1; // Start in focus_mode
  bool frequent_stats = false;

  bool output_progress = true;
  bool output_stats = true;
  bool use_sections = false;

  string dump_dot_path;
  char filter_sep = ',';
  bool use_new_enumer = true;
#ifdef BUILD_FINDMUS_EXAMPLES
  string demo_name;
  string demo_path;
  int demo_rand_seed;
  int demo_rand_cons;
  int demo_rand_muses;
  int demo_rand_mus_size;
#endif
  bool list_solvers = false;
  bool list_solvers_json = false;

  bool compile_verbose = false;
  bool compile_stats = false;
  bool compile_domains = false;

  DriverOptions() {
#ifdef BUILD_FINDMUS_EXAMPLES
    demo_rand_seed = (int)time(NULL);
#endif
  }

  void print_progress(float f) const;

  void print_intermediate_stats(
    std::chrono::time_point<std::chrono::system_clock> start_time,
    Statistics& stats, int sat_calls_d) const;

  void print_totals(
      std::chrono::time_point<std::chrono::system_clock> start_time,
      Statistics& stats) const;

  void print(const std::string& msg) const;
};

class MUSEnumOptions {
public:
  enum LOG_SOURCE {LOG_MAP, LOG_ENUM, LOG_SOLVE};

  bool verbose_final_stats = true;
  unsigned int verbose_map = 0;
  unsigned int verbose_enum = 0;
  unsigned int verbose_subsolve = 0;
  bool output_stats = true;
  bool use_sections = false;

  std::chrono::duration<double> timelimit;
  bool print_leftover = true;

  bool restarts_enabled = false;

  // FznSubProblem
  string mzn_stdlib_dir;
  bool subproblem_hard_functional_constraints = true;
  bool subproblem_hard_domain_constraints =
      false; // This should be false until context-find is implemented!

  bool subproblem_named_only = false;
  bool oracle_only = false;

  FilterMode subproblem_filter_mode = FILTER_FOREGROUND;
  std::vector<string> subproblem_name_filters;
  std::vector<string> subproblem_name_filters_excludes;
  std::vector<string> subproblem_path_filters;
  std::vector<string> subproblem_path_filters_excludes;
  InitialStructure subproblem_structure = STR_NORMAL;
  Binarize subproblem_binarize = BIN_ALL;
  bool subproblem_native_shrink = false;
  bool subproblem_native_shrink_ignore_min = false;
  bool subproblem_dump_timeouts = false;

  string subproblem_solver = "gecode_presolver";
  string subproblem_solver_flags = "";
  std::chrono::duration<double> subproblem_solver_time_limit =
      std::chrono::seconds(1);
  bool subproblem_adapt_time_limit = false;
  SubProblemOutputFormat subproblem_output_format = OUT_NORMAL;

  // SubsetMap
  MapDepth map_depth =
      DEPTH_INSTANCE; // DEPTH_INSTANCE should probably be the default in future
  unsigned int map_depth_max = 1;

  MusAlg map_enumeration_alg = ALG_MARCO;
  ShrinkAlg map_shrink_alg = SH_MAP_QX;
  bool map_shrink_frontier = false;
  bool map_enum_focus_mode = true;

  Statistics &stats;

  int rand_seed;
  std::default_random_engine rand_generator;
  std::uniform_int_distribution<int> rand_bin;

  MUSEnumOptions() = delete;
  MUSEnumOptions(Statistics &s)
      : timelimit{-1}, stats(s), rand_seed((int)time(NULL)),
        rand_generator(rand_seed), rand_bin(0, 1) {}
  MUSEnumOptions &operator=(const MUSEnumOptions &mo) = delete;

  void setRandSeed(int s) {
    rand_seed = s;
    rand_generator.seed(rand_seed);
  }

  bool getRandBool(void) { return rand_bin(rand_generator) == 1; }

  bool timedOut() {
    if (timelimit.count() < 0)
      return false;
    stats.last_time = std::chrono::system_clock::now();
    std::chrono::duration<double> dur = stats.last_time - stats.start_time;
    return dur > timelimit;
  }

  void adjustSolverTimeout() {
    if (timelimit.count() < 0)
      return;
    std::chrono::duration<double> elapsedTime =
        std::chrono::system_clock::now() - stats.start_time;
    std::chrono::duration<double> remainingTime = timelimit - elapsedTime;

    if (remainingTime.count() < 0)
      return;

    if (remainingTime < subproblem_solver_time_limit) {
      std::cerr << "Options: Tightening timeout to: " << remainingTime.count()
                << " seconds" << std::endl;
      subproblem_solver_time_limit = remainingTime;
    }
  }

  void solution(const std::string& msg) const;
  void log(LOG_SOURCE source, const std::string& msg, unsigned int level) const;
  void error(LOG_SOURCE source, const std::string& msg) const;

};

namespace OptionsHelper {
void help_short(int exit_code = EXIT_SUCCESS);
void help_long(void);
void parse(DriverOptions &d, MUSEnumOptions &m, int argc, const char **argv);
void parse(DriverOptions &d, MUSEnumOptions &m,
           const std::vector<std::string> &args);
vector<string> expandParamSet(const vector<string> &in_args);
} // namespace OptionsHelper

} // namespace HierMUS

#endif

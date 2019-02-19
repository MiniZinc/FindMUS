#ifndef __HIERMUS_OPTIONS_H_
#define __HIERMUS_OPTIONS_H_

#include "Types.h"

#include <string>

namespace HierMUS {

class DriverOptions {
public:
  string fznpath;
  string pathpath;
  int maxmuses = 1; // Start in focus_mode
  bool frequent_stats = false;
  bool output_progress = true;
  string dump_dot_path;
  char filter_sep = ',';
  bool ignore_sat_model = false;
  bool ignore_unsatisfiable_background = false;
  bool colour = false;
#ifdef BUILD_FINDMUS_EXAMPLES
  string demo_name;
#endif
  bool list_solvers = false;
  bool list_solvers_json = false;

  void toJSON(std::ostream& ss) {
    ss << "\"driver_options\": {\n"
       << "\t\"fznpath\": \"" << fznpath << "\",\n"
       << "\t\"fznpath\": \"" << fznpath << "\",\n"
       << "\t\"maxmuses\": " << maxmuses << ",\n"
       << "\t\"frequent_stats\": " << frequent_stats << ",\n"
       << "\t\"output_progress\": " << output_progress << ",\n"
       << "\t\"dump_dot_path\": \"" << dump_dot_path << "\",\n"
       << "\t\"filter_sep\": \"" << filter_sep << "\",\n"
       << "\t\"ignore_sat_model\": " << ignore_sat_model << ",\n"
       << "\t\"ignore_unsatisfiable_background\": " << ignore_unsatisfiable_background << ",\n"
       << "\t\"colour\": " << colour << ",\n"
#ifdef BUILD_FINDMUS_EXAMPLES
       << "\t\"demo_name\": \"" << demo_name << "\",\n"
#endif
       << "\t\"list_solvers\": " << list_solvers << ",\n"
       << "\t\"list_solvers_json\": " << list_solvers_json << ",\n"
       << "}";
  }
};

class MUSEnumOptions {
public:
  bool verbose_final_stats = true;
  unsigned int verbose_map = 0;
  unsigned int verbose_enum = 0;
  unsigned int verbose_subsolve = 0;

  double timelimit = -1;

  // FznSubProblem
  string mzn_stdlib_dir;
  bool subproblem_hard_functional_constraints = true;
  bool subproblem_hard_domain_constraints = false; // This should be false until context-find is implemented!
  bool subproblem_named_only = false;
  std::set<string> subproblem_name_filters;
  std::set<string> subproblem_name_filters_excludes;
  std::set<string> subproblem_path_filters;
  std::set<string> subproblem_path_filters_excludes;
  InitialStructure subproblem_structure = STR_NORMAL;
  Binarize subproblem_binarize = BIN_NONE;

  string subproblem_solver = "org.gecode.gecode";
  string subproblem_solver_flags = "";
  int subproblem_solver_time_limit = 1000;
  SubProblemOutputFormat subproblem_output_format = OUT_NORMAL;

  // SubsetMap
  MapDepth map_depth = DEPTH_CUSTOM; // DEPTH_INSTANCE should probably be the default in future
  unsigned int map_depth_max = 1;

  MusAlg map_enumeration_alg = ALG_STACKMUS;
  ShrinkAlg map_shrink_alg = SH_LIN;
  bool map_enum_focus_mode = true;

  Statistics& stats;

  MUSEnumOptions(Statistics& s) : stats(s) {}
  MUSEnumOptions& operator=(const MUSEnumOptions& mo) =  delete;

  bool timedOut() {
    if(timelimit < 0) return false;
    stats.last_time = wallClockTime();
    return (stats.last_time - stats.start_time > (timelimit / 1000));
  }

  void adjustSolverTimeout() {
    if(timelimit < 0) return;
    double time = wallClockTime();
    double elapsedTime = time - stats.start_time;
    double remainingTime = timelimit - elapsedTime;

    if(remainingTime < (subproblem_solver_time_limit)) {
      std::cerr << "Options: Tightening timeout to: " << (remainingTime) << " (ms)" << std::endl;
      subproblem_solver_time_limit = int(std::ceil(remainingTime));
    }
  }
};

namespace OptionsHelper {
void help_short(int exit_code = EXIT_SUCCESS);
void help_long(void);
void parse(DriverOptions& d, MUSEnumOptions& m, int argc, char**argv);
}

}

#endif

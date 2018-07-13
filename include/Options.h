#ifndef __HIERMUS_OPTIONS_H_
#define __HIERMUS_OPTIONS_H_

#include "Types.h"

#include <string>

namespace HierMUS {

  class MUSEnumOptions {
    public:
      bool verbose_final_stats = true;
      unsigned int verbose_map = 0;
      unsigned int verbose_enum = 0;
      unsigned int verbose_subsolve = 0;

      double timeout = 1800.0;

      // FznSubProblem
      string mzn_stdlib_dir;
      bool subproblem_hard_functional_constraints = true;
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
      int map_depth_max = 1;

      MusAlg map_enumeration_alg = ALG_STACKMUS;
      ShrinkAlg map_shrink_alg = SH_LIN;
      bool map_enum_focus_mode = true;

      MUSEnumOptions& operator=(const MUSEnumOptions& mo) =  delete;

      bool timedOut(Statistics& s) {
        s.last_time = wallClockTime();
        return (s.last_time - s.start_time > timeout);
      }
  };
}

#endif

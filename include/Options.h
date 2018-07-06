#ifndef __HIERMUS_OPTIONS_H_
#define __HIERMUS_OPTIONS_H_

#include "Types.h"

#include <string>

namespace HierMUS {

  class MUSEnumOptions {
    public:
      bool verbose_final_stats = true;
      bool verbose_map = false;
      bool verbose_enum = false;
      bool verbose_enum_iteration_stats = false;
      bool verbose_subsolve = false;
      bool verbose_subsolve_extra = false;

      double timeout = 1800.0;

      // FznSubProblem
      bool subproblem_hard_functional_constraints = true;
      bool subproblem_named_only = false;
      std::set<string> subproblem_name_filters;
      std::set<string> subproblem_name_filters_excludes;
      std::set<string> subproblem_path_filters;
      std::set<string> subproblem_path_filters_excludes;
      InitialStructure subproblem_structure = STR_NORMAL;
      Binarize subproblem_binarize = BIN_NONE;

      string subproblem_solver            = "org.minizinc.mzn-fzn";
      //string subproblem_solver            = "mzn-gurobi";
      string subproblem_solver_flags      = "--fzn-cmd fzn-gecode"; //"-time 1000"; // Default timeout for gecode to 1000ms
      //string subproblem_solver_flags      = "--timeout 1"; // Default timeout for gecode to 1000ms
      int    subproblem_solver_time_limit = 1100;         // Default timeout for gecode to 1000ms
      SubProblemOutputFormat subproblem_output_format = OUT_NORMAL;

      // SubsetMap
      MapDepth map_depth = DEPTH_INSTANCE;
      int map_depth_max = 0;
      bool output_minizinc = false;

      MusAlg map_enumeration_alg = ALG_STACKMUS;
      ShrinkAlg map_shrink_alg = SH_LIN;

      MUSEnumOptions& operator=(const MUSEnumOptions& mo) =  delete;

      bool timedOut(Statistics& s) {
        s.last_time = wallClockTime();
        return (s.last_time - s.start_time > timeout);
      }
  };
}

#endif

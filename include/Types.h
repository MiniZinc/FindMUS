#ifndef __HIERMUS_TYPES_H_
#define __HIERMUS_TYPES_H_

#include <set>
#include <string>
#include <unordered_map>
#include <chrono>

#include "Node.h"
#include "Selection.h"

#include <minizinc/model.hh>

namespace HierMUS {

  struct Statistics {

    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::chrono::time_point<std::chrono::system_clock> last_time;

    int sat_calls = 0;
    int map_calls = 0;

    counts treecounts;

    int local_sat_chain_limit = 0;
    int local_sat_chain = 0;
    bool should_restart = false;
    bool restarts_enabled = false;

    int nmuses = 0;

    Statistics() : start_time{std::chrono::system_clock::now()}, last_time{start_time} {}

    inline void addCounts(counts& tc) {
      treecounts = tc;
      local_sat_chain_limit = treecounts.nleaves/2;
    }

    inline void madeSatCheck(void) { sat_calls++; }
    inline void madeMapCall(void) { map_calls++; }

    inline void foundSatSet(void) {
      local_sat_chain++;
      if(restarts_enabled && shouldRestart())
        should_restart = true;
    }

    inline void foundUnSatSet(void) { local_sat_chain = 0; }

    inline bool shouldRestart(void) const {
      if(!restarts_enabled) return false;
      if(should_restart) return true;
      if(local_sat_chain_limit == 0) return local_sat_chain > local_sat_chain_limit;
      return false;
    }

  };

  std::ostream& operator<<(std::ostream& os, Statistics const& a);

  enum SubProblemOutputFormat {
    OUT_DEBUG,
    OUT_NORMAL,
    OUT_HTML,
    OUT_JSON
  };

  enum MusAlg { ALG_MARCO, ALG_REMUS }; // Other options have been removed

  enum ShrinkAlg { SH_LIN, SH_MAP_LIN, SH_QX, SH_QX2, SH_MAP_QX, SH_NATIVE };

  enum InitialStructure {
    STR_FLAT,    // Remove all structure
    STR_NORMAL,  // Leave the structure as is
    STR_GEN,     // Remove instance specific structure (loop iterations etc...)
    STR_GEN_MIX, // Place normal structure below generalized model structure
    STR_IDX,     // Remove location information
    STR_IDX_MIX  // Remove location information, include original path afterwards
  };

  enum Binarize {
    BIN_NONE,
    BIN_ALL   // Force entire tree to be binary (use with --flat-structure to remove all original instance structure)
  };

  enum MapDepth { 
    DEPTH_INSTANCE, // Don't leave the user's .mzn file
    DEPTH_PROGRAM,  // Search to the leaves of the subproblem
    DEPTH_CUSTOM    // Search to a user-specified depth
  };

  enum FilterMode {
    FILTER_FOREGROUND,
    FILTER_EXCLUSIVE
  };

  struct UnsatSet {
    struct Constraint {
      std::vector<int> indices;
      std::vector<std::string> paths;
      std::vector<std::string> constraint_names;
      std::vector<std::string> expression_names;
    };
    std::vector<Constraint> constraints;

  };

  struct ConstraintInfo {
    std::string leaf_name;
    std::string name;
    std::string assigns;
    std::string path;
    std::string expression_name;
    std::string constraint_name;

    void setAnnotatedNamesFrom(const MiniZinc::ConstraintI* ci);
  };

  class ConstraintSet {
  public:
    std::unordered_map<std::string, std::vector<ConstraintInfo> > constraints;

    ConstraintSet();
    void addConstraintInfo(const std::string& path, const ConstraintInfo& ci);

    std::string getSummary(SubProblemOutputFormat format);

  private:
    std::string getLongSummary(void);
    std::string getShortSummary(const std::string& sep = "\n");
    std::string getJSONSummary(void);
    std::string getHTMLSummary(void);
  };

  struct ShrunkSet {
    std::set<std::string> con_ids;
    bool minimal;

    ShrunkSet() : minimal{false} {};
  };

}
#endif

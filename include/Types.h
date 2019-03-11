#ifndef __HIERMUS_TYPES_H_
#define __HIERMUS_TYPES_H_

#include <set>
#include <string>
#include <unordered_map>

#include "Node.h"
#include "Selection.h"

#include <minizinc/model.hh>

namespace HierMUS {

  struct Statistics {
    double start_time;
    double last_time;
    int sat_calls;
    int map_calls;

    Statistics() : start_time{wallClockTime()}, last_time{start_time}, sat_calls{0}, map_calls{0} {}
  };

  std::ostream& operator<<(std::ostream& os, Statistics const& a);

  enum SubProblemOutputFormat {
    OUT_DEBUG,
    OUT_NORMAL,
    OUT_HTML,
    OUT_JSON
  };

  enum MusAlg { ALG_STACKMUS, ALG_MARCO, ALG_REMUS };

  enum ShrinkAlg { SH_LIN, SH_MAP_LIN, SH_QX, SH_MAP_QX };

  enum InitialStructure {
    STR_FLAT,   // Remove all structure
    STR_NORMAL, // Leave the structure as is
    STR_GEN,    // Remove instance specific structure (loop iterations etc...)
    STR_GEN_MIX // Place normal structure below generalized model structure
  };

  enum Binarize {
    BIN_NONE,
    BIN_LEAVES,    // Introduce binary structure at the program-level leaves
    BIN_EVERYWHERE // Force entire tree to be binary (use with --flat-structure to remove all original instance structure)
  };

  enum MapDepth { 
    DEPTH_INSTANCE, // Don't leave the user's .mzn file
    DEPTH_PROGRAM,  // Search to the leaves of the subproblem
    DEPTH_CUSTOM    // Search to a user-specified depth
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

}
#endif

#ifndef __HIERMUS_TYPES_H_
#define __HIERMUS_TYPES_H_

#include <set>
#include <string>
#include <unordered_map>

#include "Node.h"

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

  enum ShrinkAlg { SH_LIN, SH_QX };

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

  struct ExpandedNode {
    MapNode* parent;
    MapNode* child;

    explicit ExpandedNode(MapNode* p, MapNode* c) : parent(p), child(c) {}
    explicit ExpandedNode(MapNode* c) : parent(c), child(c) {}
    ~ExpandedNode() {}

    bool operator<(const ExpandedNode& other) const {
      return parent < other.parent || (parent == other.parent && child < other.child);
    }
    bool operator==(const ExpandedNode& other) const {
      return parent == other.parent && child == other.child;
    }
  };

  struct Selection {
    std::set<MapNode*> selected;
    std::set<ExpandedNode> include;
    std::set<MapNode*> exclude;
    bool is_min;

    Selection(const std::set<MapNode*>& s,
              const std::set<ExpandedNode>& i,
              const std::set<MapNode*>& e,
              bool m = true) 
      : selected(s),
        include(i),
        exclude(e),
        is_min(m) {}
    Selection() : is_min(true) {}
  };

  static const Selection empty_selection;

  std::set<std::string> getLeaves(const Selection& b);
  bool isLeaves(const Selection& s);

  std::ostream& operator<<(std::ostream& os, std::set<MapNode*> const& mns);
  std::ostream& operator<<(std::ostream& os, Selection const& a);
  std::ostream& operator<<(std::ostream& os, std::set<ExpandedNode> const& inc);
  std::string printMapNode(bool pol, const std::string& prefix, const MapNode* mn);
  std::ostream& streamMapNodeSet(std::ostream& os, std::set<MapNode*> const& mns, bool pol, std::string prefix);

  void debugPrint(HierMUS::Selection& s);

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

#ifndef __HIERMUS_NODE_H_
#define __HIERMUS_NODE_H_


#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <iostream>
#include <set>

#include <unordered_map>
#include "string_utils.h"


#define SolverVar int

namespace HierMUS {
  using std::string;
  using std::vector;

  static const char MINOR_SEP = '|';
  static const char MAJOR_SEP = ';';

  struct counts {
    int nleaves;
    int nbranches;
    int maxdepth;
    counts() : nleaves(0), nbranches(0), maxdepth(0) {}
  };

  struct HierVar {
    union {
      SolverVar leaf;
      struct {
        SolverVar conj;
        SolverVar disj;
        SolverVar eq;
      };
    };
    bool isLeaf;

    HierVar() : conj(0), disj(0), eq(0), isLeaf(false) {}
    HierVar(SolverVar lv) : leaf(lv), isLeaf(true) {}
    HierVar(SolverVar c, SolverVar d, SolverVar e) : conj(c), disj(d), eq(e), isLeaf(false) {}
  };

  struct MapNode {
    std::string path;
    std::string con_id;
    HierVar var;
    std::vector<MapNode> children;

    explicit MapNode(std::string p) : path(p) {}
    explicit MapNode(std::string p, std::vector<MapNode > cs) : path(p), children(cs) {}
    explicit MapNode(std::string p, HierVar v) : path(p), var(v) {}
    explicit MapNode(std::string p, HierVar v, std::vector<MapNode > cs) : path(p), var(v), children(cs) {}
    explicit MapNode(std::string p, std::string c) : path(p), con_id(c) {}
    explicit MapNode(std::string p, std::string c, HierVar v) : path(p), con_id(c), var(v) {}
    explicit MapNode(std::string p, std::string c, HierVar v, std::vector<MapNode > cs) : path(p), con_id(c), var(v), children(cs) {}
    MapNode() {}
    ~MapNode() {}

    MapNode& addPath(std::vector<std::string>& splitpath, unsigned int p);
    MapNode& addPath(std::string path);
    counts getCounts(bool complete = true) const;
    void getCounts(counts& cs, int depth = 1) const;
    void getIncompleteCounts(counts& cs, int depth = 1) const;
    void compact();
    void mergeLeaves();
    void makeBinary(std::function<bool(const MapNode&)> cond);
  };

  void getLeaves(const MapNode& node, std::set<std::string>& leaves);
  std::ostream& operator<<(std::ostream& os, std::set<MapNode*> const& mns);
  std::string printMapNode(bool pol, const std::string& prefix, const MapNode* mn);
  std::ostream& streamMapNodeSet(std::ostream& os, std::set<MapNode*> const& mns, bool pol, std::string prefix);

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

  std::ostream& operator<<(std::ostream& os, std::set<ExpandedNode> const& inc);
  std::ostream& streamExpandedNodeSet(std::ostream& os, std::set<ExpandedNode> const& mns, bool pol, std::string prefix);

  class DotWriter {
    std::ostream& os;
    const MapNode& root;
    std::vector<std::string> names;
    std::unordered_map<std::string, size_t> pathToIdx;

    struct C {
      size_t l;
      size_t r;
    };

    vector<C> connections;

    void collect(const MapNode& n) {
      size_t idx = pathToIdx[n.path];
      for(const MapNode& c : n.children) {
        size_t c_idx = names.size();
        pathToIdx[c.path] = c_idx;
        names.push_back(c.path + " : " + c.con_id);
        connections.push_back({idx, c_idx});
        collect(c);
      }
    }
    string simplify(const string& path) {
      vector<string> splitPath = utils::split(path, MAJOR_SEP);
      vector<string> backs;
      for(const string& el : splitPath) {
        vector<string> splitEl = utils::split(el, MINOR_SEP);
        backs.push_back(splitEl.back());
      }
      return utils::join(backs, string(1, MAJOR_SEP));
    }


    public:
    DotWriter(std::ostream& o, const MapNode& r) : os(o), root(r) {
      size_t idx = names.size();
      pathToIdx[root.path] = idx;
      names.push_back(root.path);
      collect(root);
    }

    void print() {
      os << "digraph Paths {\n";

      for(size_t i=0; i<names.size(); i++) {
        os << "  a" << i << "[label=\"" << simplify(names[i]) << "\"];\n";
      }

      for(const C& c : connections) {
        os << "  a" << c.l << " -> a" << c.r << ";\n";
      }

      os << "}\n";
    }
  };
}


#endif

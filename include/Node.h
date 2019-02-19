#ifndef __HIERMUS_NODE_H_
#define __HIERMUS_NODE_H_

#include <chuffed/vars/modelling.h>

#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <iostream>

#include <unordered_map>
#include "string_utils.h"

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
      BoolView* leaf;
      struct {
        BoolView* conj;
        BoolView* disj;
        BoolView* eq;
      };
    };
    bool isLeaf;

    HierVar() : conj(NULL), disj(NULL), eq(NULL), isLeaf(false) {}
    HierVar(BoolView* lv) : leaf(lv), isLeaf(true) {}
    HierVar(BoolView* c, BoolView* d, BoolView* e) : conj(c), disj(d), eq(e), isLeaf(false) {}
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

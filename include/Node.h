#ifndef __HIERMUS_NODE_H_
#define __HIERMUS_NODE_H_

#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "string_utils.h"
#include <unordered_map>

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
  HierVar(SolverVar c, SolverVar d, SolverVar e)
      : conj(c), disj(d), eq(e), isLeaf(false) {}
};

struct MapNode {
  std::string path;
  std::string con_id;
  HierVar var;
  std::vector<MapNode> children;

  explicit MapNode(std::string p) : path(p) {}
  explicit MapNode(std::string p, std::vector<MapNode> cs)
      : path(p), children(cs) {}
  explicit MapNode(std::string p, HierVar v) : path(p), var(v) {}
  explicit MapNode(std::string p, HierVar v, std::vector<MapNode> cs)
      : path(p), var(v), children(cs) {}
  explicit MapNode(std::string p, std::string c) : path(p), con_id(c) {}
  explicit MapNode(std::string p, std::string c, HierVar v)
      : path(p), con_id(c), var(v) {}
  explicit MapNode(std::string p, std::string c, HierVar v,
                   std::vector<MapNode> cs)
      : path(p), con_id(c), var(v), children(cs) {}
  MapNode() {}
  ~MapNode() {}

  MapNode &addPath(std::vector<std::string> &splitpath, unsigned int p);
  MapNode &addPath(std::string &path);
  MapNode &getNode(std::string &path);
  counts getCounts(bool complete = true) const;
  void getCounts(counts &cs, int depth = 1) const;
  void getIncompleteCounts(counts &cs, int depth = 1) const;
  void compact();
  void mergeLeaves();
  void makeBinary(std::function<bool(const MapNode &)> cond);

  std::set<std::string> getLeaves() const;
};

class NodeSet;
std::ostream &operator<<(std::ostream &os, MapNode const &mn);
std::ostream &operator<<(std::ostream &os, NodeSet const &mns);

class NodeSet {
  protected:
    std::vector<const MapNode *> nodes;

  public:
    using iterator = typename std::vector<const MapNode *>::iterator;
    using const_iterator = typename std::vector<const MapNode *>::const_iterator;
    using reverse_iterator = typename std::vector<const MapNode *>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<const MapNode *>::const_reverse_iterator;

    NodeSet() {};
    NodeSet(std::initializer_list<const MapNode *> other) : nodes{other} {}

    size_t insert(const MapNode *node) {
      size_t idx = nodes.end() - nodes.begin();
      if(nodes.empty() || nodes.back() < node) {
        nodes.push_back(node);
      } else {
        iterator it = std::lower_bound(nodes.begin(), nodes.end(), node);
        if(it == nodes.end()) {
          nodes.push_back(node);
        } else if(*it != node) {
          idx = it - nodes.begin();
          nodes.insert(it, node);
        }
      }
      return idx;
    }

    size_t erase(const MapNode *node) {
      size_t idx = nodes.end() - nodes.begin();
      iterator it = std::lower_bound(nodes.begin(), nodes.end(), node);
      if(it != nodes.end() && *it == node) {
        idx = erase(it);
      }
      return idx;
    }

    size_t erase(iterator &it) {
      size_t idx = it - nodes.begin();
      nodes.erase(it);
      return idx;
    }

    iterator begin() { return nodes.begin(); }
    iterator end() { return nodes.end(); }

    const_iterator begin() const { return nodes.cbegin(); }
    const_iterator end() const { return nodes.cend(); }

    reverse_iterator rbegin() { return nodes.rbegin(); }
    reverse_iterator rend() { return nodes.rend(); }

    const_reverse_iterator rbegin() const { return nodes.rbegin(); }
    const_reverse_iterator rend() const { return nodes.rend(); }

    iterator find(const MapNode *node) {
      iterator it = std::lower_bound(nodes.begin(), nodes.end(), node);
      if(it != nodes.end() && *it == node)
        return it;
      return nodes.end();
    }

    const_iterator find(const MapNode *node) const {
      const_iterator it = std::lower_bound(nodes.cbegin(), nodes.cend(), node);
      if(it != nodes.end() && *it == node)
        return it;
      return nodes.end();
    }

    size_t size() const { return nodes.size(); }
    bool empty() const { return nodes.empty(); }
    void clear() { nodes.clear(); }
};



std::string printMapNode(bool pol, const std::string &prefix,
                         const MapNode *mn);
std::ostream &streamMapNodeSet(std::ostream &os, NodeSet const &mns,
                               bool pol, std::string prefix);

struct ExpandedNode {
  const MapNode *parent;
  const MapNode *child;

  explicit ExpandedNode(const MapNode *p, const MapNode *c) : parent(p), child(c) {}
  explicit ExpandedNode(const MapNode *c) : parent(c), child(c) {}
  ~ExpandedNode() {}
};

class ExpandedNodeSetIterator {
  NodeSet::const_iterator nodes_it;
  vector<const MapNode *>::const_iterator roots_it;

public:
  ExpandedNodeSetIterator(const ExpandedNodeSetIterator &ei)
    : nodes_it {ei.nodes_it}, roots_it {ei.roots_it} {}
  ExpandedNodeSetIterator(const NodeSet::const_iterator &nit,
                          const vector<const MapNode *>::const_iterator &rit)
    : nodes_it{nit}, roots_it{rit} {}
  ~ExpandedNodeSetIterator() {}

  ExpandedNodeSetIterator& operator=(const ExpandedNodeSetIterator &ei) {
    if (this != &ei) {
      nodes_it = ei.nodes_it;
      roots_it = ei.roots_it;
    }
    return *this;
  }
  bool operator==(const ExpandedNodeSetIterator &ei) const { return nodes_it == ei.nodes_it && roots_it == ei.roots_it; }
  bool operator!=(const ExpandedNodeSetIterator &ei) const { return nodes_it != ei.nodes_it || roots_it != ei.roots_it; }
  ExpandedNodeSetIterator &operator++() {
    nodes_it++;
    roots_it++;
    return *this;
  }
  ExpandedNodeSetIterator &operator--() {
    nodes_it--;
    roots_it--;
    return *this;
  }

  ExpandedNode operator*() const { return ExpandedNode {*roots_it, *nodes_it}; }
};

class ExpandedNodeSet {
  protected:
    NodeSet nodes;
    std::vector<const MapNode *> roots;

  public:

    ExpandedNodeSet() {};
    ExpandedNodeSet(const std::initializer_list<const ExpandedNode> &&other) {
      for(const ExpandedNode& en: other) {
        insert(en);
      }
    }

    void insert(const ExpandedNode& en) {
      insert(en.child, en.parent);
    }

    void insert(const MapNode *node, const MapNode *parent) {
      size_t idx = nodes.insert(node);
      roots.insert(roots.begin() + idx, parent);
    }

    void insert(const MapNode *node) {
      insert(node, node);
    }

    void erase(const MapNode *node) {
      size_t idx = nodes.erase(node);
      if (idx < roots.size()) {
        roots.erase(roots.begin() + idx);
      }
    }

    ExpandedNodeSetIterator begin() const {
      return ExpandedNodeSetIterator(nodes.begin(), roots.begin());
    }

    ExpandedNodeSetIterator end() const {
      return ExpandedNodeSetIterator(nodes.end(), roots.end());
    }

    bool contains_node(const MapNode *node) const {
      return nodes.find(node) != nodes.end();
    }

    const NodeSet& get_nodes() const { return nodes; }
    const vector<const MapNode *>& get_roots() const { return roots; }

    size_t size() const { return nodes.size(); }
    bool empty() const { return nodes.empty(); }

    void clear() {
      nodes.clear();
      roots.clear();
    }
};

std::ostream &operator<<(std::ostream &os, ExpandedNode const& en);
std::ostream &operator<<(std::ostream &os, ExpandedNodeSet const &inc);
std::ostream &streamExpandedNodeSet(std::ostream &os,
                                    ExpandedNodeSet const &mns, bool pol,
                                    std::string prefix);

class DotWriter {
  std::ostream &os;
  const MapNode &root;
  std::vector<std::string> names;
  std::unordered_map<std::string, size_t> pathToIdx;

  struct C {
    size_t l;
    size_t r;
  };

  vector<C> connections;

  void collect(const MapNode &n) {
    size_t idx = pathToIdx[n.path];
    for (const MapNode &c : n.children) {
      size_t c_idx = names.size();
      pathToIdx[c.path] = c_idx;
      names.push_back(c.path + " : " + c.con_id);
      connections.push_back({idx, c_idx});
      collect(c);
    }
  }
  string simplify(const string &path) {
    vector<string> splitPath = utils::split(path, MAJOR_SEP);
    vector<string> backs;
    for (const string &el : splitPath) {
      vector<string> splitEl = utils::split(el, MINOR_SEP);
      backs.push_back(splitEl.back());
    }
    return utils::join(backs, string(1, MAJOR_SEP));
  }

public:
  DotWriter(std::ostream &o, const MapNode &r) : os(o), root(r) {
    size_t idx = names.size();
    pathToIdx[root.path] = idx;
    names.push_back(root.path);
    collect(root);
  }

  void print() {
    os << "digraph Paths {\n";

    for (size_t i = 0; i < names.size(); i++) {
      os << "  a" << i << "[label=\"" << simplify(names[i]) << "\"];\n";
    }

    for (const C &c : connections) {
      os << "  a" << c.l << " -> a" << c.r << ";\n";
    }

    os << "}\n";
  }
};
} // namespace HierMUS

#endif

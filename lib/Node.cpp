#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Node.h"
#include "string_utils.h"

namespace HierMUS {
using std::string;
using std::vector;

MapNode &MapNode::addPath(std::vector<std::string> &splitpath, unsigned int p) {
  if (p == splitpath.size()) {
    if (!children.empty()) {
      children.push_back(MapNode("_"));
      return children.back();
    }
    return *this;
  }
  for (MapNode &child : children) {
    if (child.path == splitpath[p]) {
      return child.addPath(splitpath, p + 1);
    }
  }
  if (!con_id.empty()) {
    children.push_back(MapNode(path, con_id));
    con_id.clear();
  }
  children.push_back(MapNode(splitpath[p]));
  return children.back().addPath(splitpath, p + 1);
}

MapNode &MapNode::addPath(std::string &path) {
  vector<string> splitpath = utils::split(path, MAJOR_SEP);
  return addPath(splitpath, 0);
}

void MapNode::getCounts(counts &cs, int depth) const {
  if (depth > cs.maxdepth)
    cs.maxdepth = depth;
  if (children.size() == 0) {
    cs.nleaves++;
  } else {
    if (!con_id.empty()) {
      std::cout << "Found lost leaf: " << con_id << std::endl;
    }
    cs.nbranches++;
  }
  for (const MapNode &n : children)
    n.getCounts(cs, depth + 1);
}

void MapNode::getIncompleteCounts(counts &cs, int depth) const {
  if (depth > cs.maxdepth)
    cs.maxdepth = depth;
  if (var.isLeaf) {
    cs.nleaves++;
  } else {
    cs.nbranches++;
    for (const MapNode &n : children)
      n.getCounts(cs, depth + 1);
  }
}

counts MapNode::getCounts(bool complete) const {
  counts cs;
  if (complete) {
    getCounts(cs, 1);
  } else {
    getIncompleteCounts(cs, 1);
  }
  return cs;
}

void MapNode::compact() {
  if (children.size() == 0)
    return;
  if (children.size() == 1) {
    MapNode the_child = children[0];
    children.clear();
    std::stringstream new_path;
    if (the_child.children.size() == 0)
      con_id = the_child.con_id;
    new_path << path << MAJOR_SEP << the_child.path;
    path = new_path.str();
    for (MapNode &child : the_child.children)
      children.push_back(child);
  }
  for (MapNode &child : children)
    child.compact();
}

void MapNode::mergeLeaves() {
  std::unordered_map<std::string, vector<std::string>> paths_leaf_names;
  vector<MapNode> children_copy = children;
  children.clear();

  for (MapNode &child : children_copy) {
    if (child.children.empty()) {
      paths_leaf_names[child.path].push_back(child.con_id);
    } else {
      child.mergeLeaves();
      children.push_back(child);
    }
  }

  for (auto &p : paths_leaf_names) {
    children.push_back(MapNode(p.first, utils::join(p.second, "#")));
  }
}

void MapNode::makeBinary(std::function<bool(const MapNode &)> cond) {
  if (cond(*this)) {
    vector<MapNode> left(children.begin(),
                         children.begin() + children.size() / 2);
    vector<MapNode> right(children.begin() + (children.size() / 2),
                          children.end());
    children.clear();
    children.push_back(MapNode(path + "_L", {}, left));
    children.push_back(MapNode(path + "_R", {}, right));
  }
  for (MapNode &child : children) {
    child.makeBinary(cond);
  }
}

void getLeaves(const MapNode &node, std::set<std::string> &leaves) {
  if (!node.children.empty()) {
    for (const MapNode &child : node.children)
      getLeaves(child, leaves);
  } else {
    vector<string> split_leaves = utils::split(node.con_id, '#', false);
    for (const string &l : split_leaves)
      leaves.insert(l);
  }
}

std::ostream &operator<<(std::ostream &os, std::set<MapNode *> const &mns) {
  return streamMapNodeSet(os, mns, true, "c_");
}

std::ostream &operator<<(std::ostream &os, std::set<ExpandedNode> const &inc) {
  bool first = true;
  os << "{";
  for (const ExpandedNode &en : inc) {
    if (!first)
      os << ", ";
    os << printMapNode(true, "e_", en.child);
    first = false;
  }
  os << "}";
  return os;
}

inline std::string printMapNode(bool pol, const std::string &prefix,
                                const MapNode *mn) {
  std::stringstream ss;
  if (!pol)
    ss << "~";
  if (!mn->var.isLeaf)
    ss << prefix;
  ss << mn->path;
  return ss.str();
}

std::ostream &streamExpandedNodeSet(std::ostream &os,
                                    std::set<ExpandedNode> const &mns, bool pol,
                                    std::string prefix) {
  bool first = true;
  os << "{";
  for (const ExpandedNode &emn : mns) {
    const MapNode *mn = emn.child;
    if (!first)
      os << ", ";
    os << printMapNode(pol, prefix, mn);
    first = false;
  }
  os << "}";
  return os;
}

std::ostream &streamMapNodeSet(std::ostream &os, std::set<MapNode *> const &mns,
                               bool pol, std::string prefix) {
  bool first = true;
  os << "{";
  for (const MapNode *mn : mns) {
    if (!first)
      os << ", ";
    os << printMapNode(pol, prefix, mn);
    first = false;
  }
  os << "}";
  return os;
}
} // namespace HierMUS

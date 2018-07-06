#include <iostream>

#include "Types.h"

namespace HierMUS {

  void getLeaves(const MapNode& node, std::set<std::string>& leaves) {
    if(!node.children.empty()) {
      for(const MapNode& child : node.children)
        getLeaves(child, leaves);
    } else {
      vector<string> split_leaves = utils::split(node.con_id, '#', false);
      for(const string& l : split_leaves)
        leaves.insert(l);
    }
  }

  std::set<std::string> getLeaves(const Selection& b) {
    std::set<std::string> leaves;
    for(const MapNode* node : b.selected) {
      getLeaves(*node, leaves);
    }
    return leaves;
  }

  inline
  std::string printMapNode(bool pol, const std::string& prefix, const MapNode* mn) {
    std::stringstream ss;
    if(!pol) ss << "~";
    if(!mn->var.isLeaf) ss << prefix;
    ss << mn->path;
    return ss.str();
  }

  std::ostream& streamMapNodeSet(std::ostream& os, std::set<MapNode*> const& mns, bool pol, std::string prefix) {
    bool first = true;
    os << "{";
    for(const MapNode* mn : mns) {
      if(!first) os << ", ";
      os << printMapNode(pol, prefix, mn);
      first = false;
    }
    os << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, std::set<MapNode*> const& mns) {
    return streamMapNodeSet(os, mns, true, "c_");
  }

  std::ostream& operator<<(std::ostream& os, std::set<ExpandedNode> const& inc) {
    bool first = true;
    os << "{";
    for(const ExpandedNode& en : inc) {
      if(!first) os << ", ";
      os << printMapNode(true, "e_", en.child);
      first = false;
    }
    os << "}";
    return os;
  }

  static bool output_leaves_for_selections = false;
  static bool output_details_for_selections = true;

  std::ostream& operator<<(std::ostream& os, Selection const& a) {
    bool first = true;
    if(a.selected.empty()) {
      os << "empty";
      return os;
    }
    if (output_leaves_for_selections)  {
      std::set<std::string> leaves = getLeaves(a);
      for(const string& leaf : leaves) {
        os << (first ? " " : ", ") << leaf;
        first = false;
      }
      return os;
    }
    os << a.selected;
    if(output_details_for_selections) {
      os << " inc: " << a.include;
      os << " exc: ";
      streamMapNodeSet(os, a.exclude, false, "d_");
    }
    return os;
  }


  std::ostream& operator<<(std::ostream& os, Statistics const& s) {
    os << "map: " << s.map_calls;
    os << "\tsat: " << s.sat_calls;
    os << "\ttotal: " << s.sat_calls + s.map_calls;
    return os;
  }

  void debugPrint(Selection& s) {
    std::stringstream ss;

    ss << "<s: {";
    bool first = true;
    for(const MapNode* mn : s.selected) {
      ss << (first ? " " : ", ")
        << (mn->var.isLeaf ? "" : "c_")
        << mn->path;
      first = false;
    }
    ss << " }, ";

    ss << "in: {";
    first = true;
    for(const ExpandedNode& en : s.include) {
      ss << (first ? " " : ", ")
        << (en.child->var.isLeaf ? "" : "e_")
        << en.child->path;
      first = false;
    }
    ss << " }, ";

    ss << "ex: {";
    first = true;
    for(const MapNode* mn : s.exclude) {
      ss << (first ? " " : ", ")
        << (mn->var.isLeaf ? "" : "e_")
        << mn->path;
      first = false;
    }
    ss << " }>";
    std::cout << ss.str();
  }

  bool isLeaves(const Selection& s) {
    for(const ExpandedNode& en : s.include) if(!en.child->var.isLeaf) return false;
    return true;
  }

}


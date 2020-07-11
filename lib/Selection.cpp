#include <iostream>

#define RANDOM_SEL_SPLIT 0
#if RANDOM_SEL_SPLIT
#include <random>
#endif

#include "Selection.h"
#include "path_utils.h"

namespace HierMUS {

  std::set<std::string> getLeaves(const Selection& b) {
    std::set<std::string> leaves;
    for(const MapNode* node : b.selected) {
      getLeaves(*node, leaves);
    }
    return leaves;
  }

  Selection sel_union(const Selection& c1, const Selection& c2) {
    Selection un = c1;
    un.exclude.insert(c2.exclude.begin(), c2.exclude.end());
    for(MapNode* mn: c2.selected) {
      un.selected.insert(mn);
      un.include.insert(ExpandedNode(mn));
      un.exclude.erase(mn);
    }
    for(MapNode* mn : c1.selected) {
      un.exclude.erase(mn);
    }
    return un;
  }

  Selection sel_difference(const Selection& c1, const Selection& c2) {
    Selection di = c1;
    for(MapNode* mn : c2.selected) {
      di.selected.erase(mn);
      di.include.erase(ExpandedNode(mn));
      di.exclude.insert(mn);
    }
    return di;
  }

  Selection sel_complement(const Selection& U, const Selection& S) {
    Selection co = U;

    co.include.clear();
    co.selected.clear();

    for(MapNode* mn : U.selected) {
      if(S.selected.find(mn) == S.selected.end()) {
        co.selected.insert(mn);
        co.include.insert(ExpandedNode(mn));
        co.exclude.erase(mn);
      } else {
        co.exclude.insert(mn);
      }
    }
    return co;
  }

  void sel_split(const Selection& C, Selection& c1, Selection& c2) {
#if RANDOM_SEL_SPLIT
    // Random values should be provided by MUSEnumOptions for
    // reproducibility reasons. This code is just for small tests
    static std::random_device rd;
    static std::uniform_int_distribution<int> rand_bin(0,1);
#endif

    size_t mid = C.selected.size() / 2;
    c1.exclude.insert(C.exclude.begin(), C.exclude.end());
    c2.exclude.insert(C.exclude.begin(), C.exclude.end());

    std::set<MapNode*>::iterator it = C.selected.begin();
    for(size_t c=0; c<C.selected.size(); c++, ++it) {
#if RANDOM_SEL_SPLIT
      if(rand_bin(rd) == 1) { // c1
#else
      if(c < mid) { // c1
#endif
        c1.selected.insert(*it);
        c1.include.insert(ExpandedNode(*it));
        c2.exclude.insert(*it);
      } else { // c2
        c2.selected.insert(*it);
        c2.include.insert(ExpandedNode(*it));
        c1.exclude.insert(*it);
      }
    }
  }
  
  Selection empty_sel(const Selection& C) {
    Selection e;
    e.exclude.insert(C.selected.begin(), C.selected.end());
    e.exclude.insert(C.exclude.begin(), C.exclude.end());
    return e;
  }

  bool is_subset(const Selection& C, const std::set<MapNode*>& crits) {
    if(crits.empty() || C.selected.size() > crits.size()) return false;
    for(MapNode* mn : C.selected) {
      if(crits.find(mn) == crits.end())
        return false;
    }
    return true;
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

  bool isLeaves(const Selection& s) {
    for(const ExpandedNode& en : s.include) if(!en.child->var.isLeaf) return false;
    return true;
  }

}


#ifndef __HIERMUS_SELECTION_H_
#define __HIERMUS_SELECTION_H_

#include <set>
#include <string>
#include <unordered_map>

#include "Node.h"

#include <minizinc/model.hh>

namespace HierMUS {

  struct Selection {
    std::set<MapNode*> selected;     // previously selected
    std::set<ExpandedNode> include;  // candidates to select from
    std::set<MapNode*> exclude;      // nodes that should be assumed false
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

  inline Selection sel_union(const Selection& c1, const Selection& c2);
  inline void sel_split(const Selection& C, Selection& c1, Selection& c2);
  inline Selection empty_sel(const Selection& C);
  inline bool is_subset(const Selection& C, const std::set<MapNode*>& crits);
  inline Selection sel_complement(const Selection& original, const Selection& subset);

  std::set<std::string> getLeaves(const Selection& b);
  bool isLeaves(const Selection& s);

  std::ostream& operator<<(std::ostream& os, Selection const& a);


}
#endif

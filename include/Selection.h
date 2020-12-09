#ifndef __HIERMUS_SELECTION_H_
#define __HIERMUS_SELECTION_H_

#include <set>
#include <string>
#include <unordered_map>
#include <algorithm>

#include "Node.h"

#include <minizinc/model.hh>

namespace HierMUS {

class Selection {
protected:
  NodeSet roots;   // previously selected
  ExpandedNodeSet frontier; // candidates to select from
  NodeSet complement;    // nodes that should be assumed false
  bool is_min;

  NodeSet& selected();
  ExpandedNodeSet& included();
  NodeSet& excluded();

  void roots_erase(MapNode* mn);
  void frontier_erase(MapNode* mn);
  void complement_erase(MapNode* mn);

public:
  Selection(const NodeSet &s, const ExpandedNodeSet &i, const NodeSet &e, bool m = true);
  Selection();

  void setMinimal(bool im);
  bool knownMinimal();

  size_t size() const;
  bool empty() const;

  const NodeSet& const_selected() const;
  const ExpandedNodeSet& const_included() const;
  const NodeSet& const_excluded() const;

  void select(MapNode *mn, MapNode* parent = nullptr);
  void select(const NodeSet& nodes);

  void exclude(const NodeSet& nodes);
  void exclude(MapNode *mn);

  void expand(const Selection &m);

  void updateIncludeExclude();

  void shrinkTo(std::set<std::string> &con_ids);

  bool isSelected(MapNode* mn);

  void allow_excludes();

  std::set<std::string> getLeaves() const;
  bool isLeaves() const;

  Selection get_diff(const Selection& other) const;
  Selection get_union(const Selection& other) const;
};

static const Selection empty_selection;

Selection sel_union(const Selection& c1, const Selection& c2);
void sel_split(const Selection &C, Selection &c1, Selection &c2);
Selection empty_sel(const Selection &C);
bool is_subset(const Selection &C, const NodeSet &crits);

std::ostream &operator<<(std::ostream &os, Selection const &a);

} // namespace HierMUS
#endif

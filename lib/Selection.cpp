#include <iostream>

#define RANDOM_SEL_SPLIT 0
#if RANDOM_SEL_SPLIT
#include <random>
#endif

#include "Selection.h"
#include "path_utils.h"

namespace HierMUS {


Selection::Selection(const std::set<MapNode *> &s, const std::set<ExpandedNode> &i,
          const std::set<MapNode *> &e, bool m)
    : roots(s), frontier(i), complement(e), is_min(m) {}

Selection::Selection() : is_min(true) {}

void Selection::setMinimal(bool im) {
  is_min = im;
}

bool Selection::knownMinimal() {
  return is_min;
}

NodeSet& Selection::selected() { return roots; }
NodeSet& Selection::excluded() { return complement; }
ExpandedNodeSet& Selection::included() { return frontier; }

const NodeSet& Selection::const_selected() const { return roots; }
const NodeSet& Selection::const_excluded() const { return complement; }
const ExpandedNodeSet& Selection::const_included() const { return frontier; }

size_t Selection::size() const { return roots.size(); }
bool Selection::empty() const { return size() == 0; }

void Selection::exclude(const std::set<MapNode *>& nodes) {
  for(MapNode *mn : nodes) {
    exclude(mn);
  }
}

void Selection::exclude(MapNode *mn) {
  complement.insert(mn);

  auto sit = roots.find(mn);
  if (sit != roots.end()) roots.erase(sit);

  auto fit = frontier.find(ExpandedNode {mn});
  if (fit != frontier.end()) frontier.erase(fit);
}

void Selection::select(const NodeSet& nodes) {
  for(MapNode *mn : nodes) {
    select(mn);
  }
}

void Selection::select(MapNode *mn, MapNode* parent) {
  if(parent) {
    roots.insert(parent);
    frontier.insert(ExpandedNode {parent, mn});
  } else {
    roots.insert(mn);
    frontier.insert(ExpandedNode {mn});
  }

  auto cit = complement.find(mn);
  if (cit != complement.end()) complement.erase(cit);
}

void Selection::updateIncludeExclude() {
  std::set<ExpandedNode> old_frontier = frontier;
  frontier.clear();
  for (const ExpandedNode &en : old_frontier) {
    if (roots.find(en.child) == roots.end()) {
      complement.insert(en.child);
    } else {
      frontier.insert(en);
      complement.erase(en.child);
    }
  }
}

void Selection::shrinkTo(std::set<std::string> &con_ids) {
  std::set<MapNode *> roots_copy = roots;
  for (MapNode *mn : roots_copy) {
    std::set<string> leaves = mn->getLeaves();

    if (!any_of(con_ids.begin(), con_ids.end(), [&](const string &s) {
          return leaves.find(s) != leaves.end();
        })) {
      roots.erase(mn);
    }
  }
  updateIncludeExclude();
}

bool Selection::isSelected(MapNode* mn) {
  return roots.find(mn) != roots.end();
}

Selection Selection::get_diff(const Selection& c2) const {
  Selection diff = *this;
  for(MapNode* mn: c2.const_selected()) {
    diff.exclude(mn);
  }
  return diff;
}

Selection Selection::get_union(const Selection &c2) const {
  Selection un = *this;
  for(MapNode* mn: c2.const_selected()) {
    un.select(mn);
  }
  return un;
}

void Selection::allow_excludes() {
  for (MapNode *node : complement) {
    roots.insert(node);
    frontier.insert(ExpandedNode(node));
  }
  complement.clear();
}

std::set<std::string> Selection::getLeaves() const {
  std::set<std::string> leaves;
  for (const MapNode *node : roots) {
    std::set<std::string> node_leaves = node->getLeaves();
    leaves.insert(node_leaves.begin(), node_leaves.end());
  }
  return leaves;
}

bool Selection::isLeaves() const {
  for (const ExpandedNode &en : frontier)
    if (!en.child->var.isLeaf)
      return false;
  return true;
}

inline
bool contains_child(const ExpandedNodeSet& ens, const MapNode* child) {
  return std::any_of(
      ens.begin(), ens.end(),
      [&](const ExpandedNode &en) {
        return en.child == child;
      }
  );
}


void Selection::expand(const Selection &m) {
  for (const ExpandedNode &en : frontier) {
    if (contains_child(m.const_included(), en.child)) {
      if (roots.empty() || roots.find(en.parent) != roots.end()) {
        MapNode *nm = en.child;
        if (nm->var.isLeaf || nm->children.empty()) {
          frontier.insert(en);
        } else {
          for (MapNode &child : nm->children)
            select(&child, en.parent);
        }

      } else {
        exclude(en.child);
      }
    } else {
      select(en.child, en.parent);
    }
  }
}


Selection sel_union(const Selection& c1, const Selection& c2) {
  return c1.get_union(c2);
}


Selection sel_difference(const Selection &c1, const Selection &c2) {
  return c1.get_diff(c2);
}


void sel_split(const Selection &C, Selection &c1, Selection &c2) {
#if RANDOM_SEL_SPLIT
  // Random values should be provided by MUSEnumOptions for
  // reproducibility reasons. This code is just for small tests
  static std::random_device rd;
  static std::uniform_int_distribution<int> rand_bin(0, 1);
#endif

  size_t mid = C.size() / 2;
  c1 = C;
  c2 = C;

  std::set<MapNode *>::iterator it = C.const_selected().begin();
  for (size_t c = 0; c < C.const_selected().size(); c++, ++it) {
#if RANDOM_SEL_SPLIT
    if (rand_bin(rd) == 1) { // c1
#else
    if (c < mid) { // c1
#endif
      c1.select(*it);
      c2.exclude(*it);
    } else { // c2
      c2.select(*it);
      c1.exclude(*it);
    }
  }
}

// Construct empty Selection that still contains union of nodes
Selection empty_sel(const Selection &C) {
  Selection e;
  for(MapNode* mn : C.const_selected()) {
    e.exclude(mn);
  }
  for(MapNode* mn : C.const_excluded()) {
    e.exclude(mn);
  }
  return e;
}

bool is_subset(const Selection &C, const std::set<MapNode *> &crits) {
  if (crits.empty() || C.size() > crits.size())
    return false;
  for (MapNode *mn : C.const_selected()) {
    if (crits.find(mn) == crits.end())
      return false;
  }
  return true;
}

static bool output_leaves_for_selections = false;
static bool output_details_for_selections = true;

std::ostream &operator<<(std::ostream &os, Selection const &a) {
  bool first = true;
  if (a.const_selected().empty()) {
    os << "empty";
    return os;
  }
  if (output_leaves_for_selections) {
    std::set<std::string> leaves = a.getLeaves();
    for (const string &leaf : leaves) {
      os << (first ? " " : ", ") << leaf;
      first = false;
    }
    return os;
  }
  os << a.const_selected();
  if (output_details_for_selections) {
    os << " inc: " << a.const_included();
    os << " exc: ";
    streamMapNodeSet(os, a.const_excluded(), false, "d_");
  }
  return os;
}

} // namespace HierMUS

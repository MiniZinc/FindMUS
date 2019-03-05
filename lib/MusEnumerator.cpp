#include <sstream>
#include <string>

#include "MusEnumerator.h"
#include "ChuffedSubsetMap.h"

namespace HierMUS {
  using std::string;
  using std::set;

  // Update s so that include = selected and exclude is accurate.
  void MusEnumerator::updateIncludeExclude(Selection& s) {
    set<ExpandedNode> old_include = s.include;
    s.include.clear();
    for(const ExpandedNode& en: old_include) {
      if(s.selected.find(en.child) == s.selected.end()) {
        s.exclude.insert(en.child);
      } else {
        s.include.insert(en);
        s.exclude.erase(en.child);
      }
    }
  }

  bool MusEnumerator::shrink(Selection& m, const set<MapNode*>& c) {
    switch(mopts.map_shrink_alg) {
      case SH_LIN:     return linear_shrink(m, c);
      case SH_MAP_LIN: return linear_shrink_with_map(m, c);
      case SH_QX:      return qx(m, c);
      case SH_MAP_QX:  return qx_with_map(m, c);
    }
    assert(false);
    return false;
  }

  bool MusEnumerator::linear_shrink(Selection& model, const set<MapNode*>& criticals) { 
    set<MapNode*> selected_copy = model.selected;
    for(MapNode* mn : selected_copy) {
      if(model.selected.size() == 1) break;
      if(mopts.timedOut()) return false;
      if(criticals.find(mn) != criticals.end()) continue;
      model.selected.erase(mn);
      if(subProblem.check(model)) {
        model.selected.insert(mn);
      }
      stats.sat_calls++;
    }
    updateIncludeExclude(model);
    return true;
  }

  bool MusEnumerator::linear_shrink_with_map(Selection& model, const set<MapNode*>&) {
    Selection u = model;
    do {
      subsetMap->blockSupersets(u);
      stats.map_calls++;
      Selection m = subsetMap->getSelection(u);
      if(m.selected.empty()) break;

      stats.sat_calls++;
      if(subProblem.check(m)) {
        subsetMap->blockSubsets(m, false); // Use weak blocks
      } else {
        u = m;
      }
    } while(true);
    model = u;

    updateIncludeExclude(model);
    return true;
  }

  inline
  Selection sel_union(const Selection& c1, const Selection& c2) {
    Selection un = c1;
    un.exclude.insert(c2.exclude.begin(), c2.exclude.end());
    for(MapNode* mn: c2.selected) {
      un.selected.insert(mn);
      un.include.insert(ExpandedNode(mn));
      un.exclude.erase(mn);
    }
    return un;
  }

  inline
  void sel_split(const Selection& C, Selection& c1, Selection& c2) {
    size_t mid = C.selected.size() / 2;

    set<MapNode*>::iterator it = C.selected.begin();
    c1.exclude.insert(C.exclude.begin(), C.exclude.end());
    for(size_t i=0; i<mid; i++) {
      c1.selected.insert(*it);
      c1.include.insert(ExpandedNode(*it));
      c2.exclude.insert(*it);
      ++it;
    }
    c2.exclude.insert(C.exclude.begin(), C.exclude.end());
    for(size_t i=mid; i<C.selected.size(); i++) {
      c2.selected.insert(*it);
      c2.include.insert(ExpandedNode(*it));
      c1.exclude.insert(*it);
      ++it;
    }
  }

  inline
  Selection empty_sel(const Selection& C) {
    Selection e;
    e.exclude.insert(C.selected.begin(), C.selected.end());
    e.exclude.insert(C.exclude.begin(), C.exclude.end());
    return e;
  }

  inline
  bool is_subset(const Selection& C, const set<MapNode*>& crits) {
    if(crits.empty() || C.selected.size() > crits.size()) return false;
    for(MapNode* mn : C.selected) {
      if(crits.find(mn) == crits.end())
        return false;
    }
    return true;
  }

  inline
  Selection sel_complement(const Selection& original, const Selection& subset) {
    Selection co = original;

    co.include.clear();
    co.selected.clear();

    for(MapNode* mn : original.selected) {
      if(subset.selected.find(mn) == subset.selected.end()) {
        co.selected.insert(mn);
        co.include.insert(ExpandedNode(mn));
        co.exclude.erase(mn);
      } else {
        co.exclude.insert(mn);
      }
    }
    return co;
  }

  OptionalSelection MusEnumerator::qx_back(Selection B, size_t D, Selection C,
                                           const set<MapNode*>& criticals) {
    if(mopts.timedOut()) return OptionalSelection();

    if(D>0 && !B.selected.empty()) {
      stats.sat_calls++;
      if(!subProblem.check(B)) return empty_sel(C);
    }
    if(C.selected.size() == 1) return C;

    Selection C1, C2;
    sel_split(C, C1, C2);

    OptionalSelection D2, D1;
    if(C2.selected.size() == 1 && is_subset(C2, criticals)) {
      D2 = C2;
    } else {
      D2 = qx_back(sel_union(B, C1), C1.selected.size(), C2, criticals);
      if(!D2.has_value()) return OptionalSelection();
    }

    if(C1.selected.size() == 1 && is_subset(C1, criticals)) {
      D1 = C1;
    } else {
      D1 = qx_back(sel_union(B, D2.get()), D2.get().selected.size(), C1, criticals);
      if(!D1.has_value()) return OptionalSelection();
    }

    return sel_union(D1.get(),D2.get());
  }

  bool MusEnumerator::qx(Selection& model, const set<MapNode*>& criticals) {
    Selection B;
    OptionalSelection res = qx_back(B, 0, model, criticals);
    if(!res.has_value()) { return false; }
    model = res.get();

    updateIncludeExclude(model);
    return true;
  }

  OptionalSelection MusEnumerator::qx_back_with_map(Selection B, size_t D, Selection C,
                                                    const set<MapNode*>& criticals) {
    if(mopts.timedOut()) return OptionalSelection();

    if(D>0 && !B.selected.empty()) {
      stats.sat_calls++;
      if(subProblem.check(B)) {
        subsetMap->blockSubsets(B, false);
      } else {
        subsetMap->blockSupersets(B);
        return empty_sel(C);
      }
    }
    if(C.selected.size() == 1) return C;

    Selection C1, C2;
    C1 = subsetMap->getRandomSelection(C, B);
    if(C1.selected.empty()) return C;
    C2 = sel_complement(C, C1);

    OptionalSelection D2, D1;
    if(C2.selected.size() == 1 && is_subset(C2, criticals)) {
      D2 = C2;
    } else {
      D2 = qx_back_with_map(sel_union(B, C1), C1.selected.size(), C2, criticals);
      if(!D2.has_value()) return OptionalSelection();
    }

    if(C1.selected.size() == 1 && is_subset(C1, criticals)) {
      D1 = C1;
    } else {
      D1 = qx_back_with_map(sel_union(B, D2.get()), D2.get().selected.size(), C1, criticals);
      if(!D1.has_value()) return OptionalSelection();
    }

    return sel_union(D1.get(),D2.get());
  }

  bool MusEnumerator::qx_with_map(Selection& model, const set<MapNode*>& criticals) {
    Selection B;
    OptionalSelection res = qx_back_with_map(B, 0, model, criticals);

    if(!res.has_value()) { return false; }
    model = res.get();

    updateIncludeExclude(model);
    return true;
  }

  MusEnumerator::MusEnumerator(SubProblem& prob, MUSEnumOptions& mo, SubsetMap* m) :
    mopts(mo), subsetMap(m ? m : new ChuffedSubsetProblem(&prob, mo)), subProblem(prob) {}
  MusEnumerator::~MusEnumerator() {}

  void MusEnumerator::setUnsatCallback(std::function<void(const Selection&)> cb) {
    unsat_callback = cb;
  }

  Statistics& MusEnumerator::getStatistics(void) {
    return stats;
  }

  void MusEnumerator::printMUS(void) {
    if(current_mus.selected.empty()) return;
    subProblem.printSol(current_mus);
  }

  const Selection& MusEnumerator::getCurrentMUS(void) { return current_mus; }

  Selection MusEnumerator::getRootSelector(void) { return subsetMap->getRootSelector(); }
  Selection MusEnumerator::getLeavesSelector(void) { return subsetMap->getLeavesSelector(); }

}

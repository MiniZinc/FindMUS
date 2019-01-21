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

  bool MusEnumerator::shrink(MUSEnumOptions& mopts,
                             SubProblem* prob, Selection& model,
                             const set<MapNode*>& criticals, Statistics& stats) {
    set<MapNode*> selected_copy = model.selected;
    for(MapNode* mn : selected_copy) {
      if(model.selected.size() == 1) break;
      if(mopts.timedOut()) return false;
      if(criticals.find(mn) != criticals.end()) continue;
      model.selected.erase(mn);
      if(prob->check(model)) {
        model.selected.insert(mn);
      }
      stats.sat_calls++;
    }
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
    return std::move(un);
  }

  inline
  void sel_split(const Selection& C, Selection& c1, Selection& c2) {
    size_t mid = C.selected.size() / 2;

    set<MapNode*>::iterator it = C.selected.begin();
    c1.exclude.insert(C.exclude.begin(), C.exclude.end());
    for(int i=0; i<mid; i++) {
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
    return std::move(e);
  }


  inline
  bool is_subset(const Selection& C, const set<MapNode*>& crits) {
    if(C.selected.size() > crits.size()) return false;
    for(MapNode* mn : C.selected) {
      if(crits.find(mn) == crits.end())
        return false;
    }
    return true;
  }

  struct OptionalSelection {
    Selection v;
    bool has_v;

    OptionalSelection(void) : has_v{false} {}

    OptionalSelection(const OptionalSelection& o) {
      has_v = o.has_value();
      if(has_v) v = o.v;
    }
    OptionalSelection(const Selection& s) {
      has_v = true;
      v = s;
    }

    OptionalSelection& operator=(const OptionalSelection& o) {
      v = o.v;
      has_v = o.has_v;
      return *this;
    }

    Selection& get(void) { return v; }
    bool has_value(void) const { return has_v; }
  };

  OptionalSelection qx_back(MUSEnumOptions& mopts,
      SubProblem* prob, Selection B, Selection D, Selection C,
      const set<MapNode*>& criticals, Statistics& stats) {
    if(mopts.timedOut()) return {};

    if(!D.selected.empty())  {
      if(!B.selected.empty()) {
        stats.sat_calls++;
        if(!prob->check(B)) return empty_sel(C);
      }
    }
    if(C.selected.size() == 1) return C;

    Selection C1;
    Selection C2;
    sel_split(C, C1, C2);

    Selection B1 = sel_union(B, C1);
    OptionalSelection D2;
    if(C2.selected.size() == 1 && is_subset(C2, criticals)) {
      D2 = C2;
    } else {
      D2 = qx_back(mopts, prob, B1, C1, C2, criticals, stats);
      if(!D2.has_value()) return {};
    }
    Selection B2 = sel_union(B, D2.get());
    OptionalSelection D1;
    if(C1.selected.size() == 1 && is_subset(C1, criticals)) {
      D1 = C1;
    } else {
      D1 = qx_back(mopts, prob, B2, D2.get(), C1, criticals, stats);
      if(!D1.has_value()) return {};
    }

    return sel_union(D1.get(),D2.get());
  }

  bool MusEnumerator::qx(MUSEnumOptions& mopts,
                         SubProblem* prob, Selection& model,
                         const set<MapNode*>& criticals, Statistics& stats) {
    Selection B;
    OptionalSelection res = qx_back(mopts, prob, B, B, model, criticals, stats);
    if(!res.has_value()) { return false; }

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

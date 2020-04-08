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
    switch (mopts.map_shrink_alg) {
      case SH_MAP_QX:  return qx_with_map(m, c);
      case SH_QX:      return qx(m, c);
      case SH_MAP_LIN: return linear_shrink_with_map(m, c);
      case SH_LIN:     return linear_shrink(m, c);
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
      stats.madeSatCheck();
      if(subProblem.check(model)) {
        stats.foundSatSet();
        model.selected.insert(mn);
      } else {
        stats.foundUnSatSet();
      }
    }
    updateIncludeExclude(model);
    return true;
  }

  bool MusEnumerator::linear_shrink_with_map(Selection& model, const set<MapNode*>&) {
    Selection u = model;
    do {
      subsetMap->pushTempBlockSupersets(u);
      stats.madeMapCall();
      Selection m = subsetMap->getSelection(u);
      subsetMap->popTempBlock();
      if(m.selected.empty()) break;
      if(mopts.timedOut()) return false;

      stats.madeSatCheck();
      if(subProblem.check(m)) {
        stats.foundSatSet();
        subsetMap->blockSubsets(m);
      } else {
        stats.foundUnSatSet();
        u = m;
      }
    } while(true);
    model = u;

    updateIncludeExclude(model);
    return true;
  }

#define QXTimeOut OptionalSelection()

  OptionalSelection MusEnumerator::qx_back(Selection B, size_t D, Selection C,
                                           const set<MapNode*>& criticals) {
    if(mopts.timedOut()) return QXTimeOut;

    if(D>0 && !B.selected.empty()) {
      stats.madeSatCheck();
      if(!subProblem.check(B)) {
        stats.foundUnSatSet();
        return empty_sel(C);
      } else {
        stats.foundSatSet();
      }
    }
    if(C.selected.size() == 1) return C;

    Selection C1, C2;
    //sel_split(mopts, C, C1, C2);
    sel_split(C, C1, C2);

    OptionalSelection D2, D1;
    if(C2.selected.size() == 1 && is_subset(C2, criticals)) {
      D2 = C2;
    } else {
      D2 = qx_back(sel_union(B, C1), C1.selected.size(), C2, criticals);
      if(!D2.has_value()) return QXTimeOut;
    }

    if(C1.selected.size() == 1 && is_subset(C1, criticals)) {
      D1 = C1;
    } else {
      D1 = qx_back(sel_union(B, D2.get()), D2.get().selected.size(), C1, criticals);
      if(!D1.has_value()) return QXTimeOut;
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

#define QXLOG(X) if(mopts.verbose_enum) std::cout << string(indent*2, ' ') << "QX: " << X << std::endl;

static int indent=0;
  /* OptionalSelection MusEnumerator::qx_back_with_map(Selection B, size_t D, Selection C,
                                                    const set<MapNode*>& criticals) {
    QXLOG("START"); indent++;
    if(mopts.timedOut()) return QXTimeOut;

    if(D>0 && !B.selected.empty()) {
      stats.madeMapCall();
      bool known_sat = subsetMap->knownSat(B);
      if(!known_sat) {
        stats.madeSatCheck();
        known_sat = subProblem.check(B);
      }

      if(known_sat) {
        stats.foundSatSet();
        subsetMap->blockSubsets(B);
      } else {
        stats.foundUnSatSet();
        subsetMap->blockSupersets(B);
        QXLOG("Returning empty"); indent--;
        return empty_sel(C);
      }
    }
    if(C.selected.size() == 1) { 
      QXLOG("returning C <- |C| == 1"); indent--;
      return C;
    }

    Selection C1, C2;
    stats.madeMapCall();
    C1 = subsetMap->getRandomSelection(C, B, true);
    if(C1.selected.empty()) {
      QXLOG("returning C <- |C1| == 0"); indent--;

      return C;
    }
    C2 = sel_complement(C, C1);

    OptionalSelection D2, D1;
    if(C2.selected.size() == 1 && is_subset(C2, criticals)) {
      D2 = C2;
    } else {
      D2 = qx_back_with_map(sel_union(B, C1), C1.selected.size(), C2, criticals);
      if(!D2.has_value()) return QXTimeOut;
    }

    if(C1.selected.size() == 1 && is_subset(C1, criticals)) {
      D1 = C1;
    } else {
      D1 = qx_back_with_map(sel_union(B, D2.get()), D2.get().selected.size(), C1, criticals);
      if(!D1.has_value()) return QXTimeOut;
    }

    QXLOG("returning D1 U D2"); indent--;
    return sel_union(D1.get(),D2.get());
  } */

  /* OptionalSelection MusEnumerator::qx_back_with_map(Selection B, size_t D, Selection C,
                                                    const set<MapNode*>& criticals) {
    QXLOG("START"); indent++;
    if(mopts.timedOut()) return QXTimeOut;

    if(D>0 && !B.selected.empty()) {
      stats.madeSatCheck();
      if(subProblem.check(B)) {
        stats.foundSatSet();
        subsetMap->blockSubsets(B);
      } else {
        stats.foundUnSatSet();
        subsetMap->blockSupersets(B);
        QXLOG("Returning empty"); indent--;
        return empty_sel(C);
      }
    }
    if(C.selected.size() == 1) { 
      QXLOG("returning C <- |C| == 1"); indent--;
      return C;
    }

    Selection C1, C2;
    stats.madeMapCall();
    C1 = subsetMap->getRandomSelection(C, B, true);
    if(C1.selected.empty()) {
      QXLOG("returning C <- |C1| == 0"); indent--;

      return C;
    }
    C2 = sel_complement(C, C1);

    OptionalSelection D2, D1;
    if(C2.selected.size() == 1 && is_subset(C2, criticals)) {
      D2 = C2;
    } else {
      D2 = qx_back_with_map(sel_union(B, C1), C1.selected.size(), C2, criticals);
      if(!D2.has_value()) return QXTimeOut;
    }

    if(C1.selected.size() == 1 && is_subset(C1, criticals)) {
      D1 = C1;
    } else {
      D1 = qx_back_with_map(sel_union(B, D2.get()), D2.get().selected.size(), C1, criticals);
      if(!D1.has_value()) return QXTimeOut;
    }

    QXLOG("returning D1 U D2"); indent--;
    return sel_union(D1.get(),D2.get());
  } */


  OptionalSelection MusEnumerator::qx_back_with_map(Selection B, size_t D, Selection C,
                                                    const set<MapNode*>& criticals) {
    QXLOG("START"); indent++;
    if(mopts.timedOut()) return QXTimeOut;

    if(D>0 && !B.selected.empty()) {
      stats.madeMapCall();
      if(!subsetMap->getSelection(B).selected.empty()) {
        stats.madeSatCheck();
        if(subProblem.check(B)) {
          stats.foundSatSet();
          subsetMap->blockSubsets(B);
        } else {
          stats.foundUnSatSet();
          subsetMap->blockSupersets(B);
          QXLOG("Returning empty"); indent--;
          return empty_sel(C);
        }
      }
    }

    if(C.selected.size() == 1) { 
      QXLOG("returning C <- |C| == 1"); indent--;
      return C;
    }

    Selection C1, C2;
    stats.madeMapCall();
    C1 = subsetMap->getRandomSelection(C, B, true);
    if(C1.selected.empty()) {
      QXLOG("returning C <- |C1| == 0"); indent--;

      return C;
    }
    C2 = sel_complement(C, C1);

    OptionalSelection D2, D1;
    if(C2.selected.size() == 1 && is_subset(C2, criticals)) {
      D2 = C2;
    } else {
      D2 = qx_back_with_map(sel_union(B, C1), C1.selected.size(), C2, criticals);
      if(!D2.has_value()) return QXTimeOut;
    }

    if(C1.selected.size() == 1 && is_subset(C1, criticals)) {
      D1 = C1;
    } else {
      D1 = qx_back_with_map(sel_union(B, D2.get()), D2.get().selected.size(), C1, criticals);
      if(!D1.has_value()) return QXTimeOut;
    }

    QXLOG("returning D1 U D2"); indent--;
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

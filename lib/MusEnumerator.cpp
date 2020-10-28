#include <sstream>
#include <string>

#include "MinisatSubsetMap.h"
#include "MusEnumerator.h"

namespace HierMUS {
using std::set;
using std::string;

// Update s so that include = selected and exclude is accurate.
void MusEnumerator::updateIncludeExclude(Selection &s) {
  set<ExpandedNode> old_include = s.include;
  s.include.clear();
  for (const ExpandedNode &en : old_include) {
    if (s.selected.find(en.child) == s.selected.end()) {
      s.exclude.insert(en.child);
    } else {
      s.include.insert(en);
      s.exclude.erase(en.child);
    }
  }
}

bool MusEnumerator::shrink(Selection &m, const set<MapNode *> &c) {
  switch (mopts.map_shrink_alg) {
  case SH_MAP_QX:
    return qx_with_map(m, c);
  case SH_QX:
    return qx(m, c);
  case SH_QX2:
    return qx2(m, c);
  case SH_MAP_LIN:
    return linear_shrink_with_map(m, c);
  case SH_LIN:
    return linear_shrink(m, c);
  }
  assert(false);
  return false;
}

bool MusEnumerator::process_native(Selection &model) {
  if (!mopts.subproblem_native_shrink)
    return false;

  ShrunkSet shrunk = subProblem.getShrunk();

  if (shrunk.con_ids.empty()) {
    std::cerr << "MusEnumerator::native_shrink: Not supported." << std::endl;
    exit(EXIT_FAILURE);
  }

  if (mopts.verbose_enum)
    std::cout << "MusEnumerator::native_shrink"
              << std::endl; //:\tfrom: " << model;

  set<string> &con_ids = shrunk.con_ids;
  if (mopts.map_shrink_frontier) {
    set<MapNode *> selected_copy = model.selected;

    for (MapNode *mn : selected_copy) {
      set<string> leaves;
      getLeaves(*mn, leaves);

      if (!any_of(con_ids.begin(), con_ids.end(), [&](const string &s) {
            return leaves.find(s) != leaves.end();
          })) {
        model.selected.erase(mn);
      }
    }

    updateIncludeExclude(model);

    return false;
  }

  // This just works on the flat leaf nodes
  model = subsetMap->convertConIds(con_ids);

  return shrunk.minimal;
}

bool MusEnumerator::linear_shrink(Selection &model,
                                  const set<MapNode *> &criticals) {
  if (process_native(model))
    return true;

  set<MapNode *> selected_copy = model.selected;
  for (MapNode *mn : selected_copy) {
    if (model.selected.size() == 1)
      break;
    if (mopts.timedOut())
      return false;
    if (criticals.find(mn) != criticals.end())
      continue;
    if (model.selected.find(mn) == model.selected.end())
      continue;

    model.selected.erase(mn);
    stats.madeSatCheck();
    if (subProblem.check(model)) {
      stats.foundSatSet();
      model.selected.insert(mn);
    } else {
      stats.foundUnSatSet();
      if (process_native(model))
        return true;
    }
  }
  updateIncludeExclude(model);
  return true;
}

bool MusEnumerator::linear_shrink_with_map(Selection &model,
                                           const set<MapNode *> &) {
  if (process_native(model))
    return true;

  Selection u = model;
  do {
    subsetMap->pushTempBlockSupersets(u);
    stats.madeMapCall();
    Selection m = subsetMap->getSelection(u);
    subsetMap->popTempBlock();

    if (m.selected.empty())
      break;
    if (mopts.timedOut())
      return false;

    stats.madeSatCheck();
    if (subProblem.check(m)) {
      stats.foundSatSet();
      subsetMap->blockSubsets(m);
    } else {
      stats.foundUnSatSet();
      if (process_native(m))
        return true;
      u = m;
    }
  } while (true);
  model = u;

  updateIncludeExclude(model);
  return true;
}

#define QXTimeOut OptionalSelection()
#define QXEarlyMin(S) OptionalSelection(S, true, true);
#define QXEarlyNonMin(S) OptionalSelection(S, true, false);

OptionalSelection MusEnumerator::qx_back(Selection B, size_t D, Selection C,
                                         const set<MapNode *> &criticals) {
  if (mopts.timedOut())
    return QXTimeOut;

  if (D > 0 && !B.selected.empty()) {
    stats.madeSatCheck();
    if (!subProblem.check(B)) {
      stats.foundUnSatSet();

      if (mopts.subproblem_native_shrink) {
        if (process_native(B))
          return QXEarlyMin(B);
        return QXEarlyNonMin(B);
      }

      return empty_sel(C);
    } else {
      stats.foundSatSet();
    }
  }
  if (C.selected.size() == 1)
    return C;

  Selection C1, C2;
  sel_split(C, C1, C2);

  OptionalSelection D2, D1;
  if (C2.selected.size() == 1 && is_subset(C2, criticals)) {
    D2 = C2;
  } else {
    D2 = qx_back(sel_union(B, C1), C1.selected.size(), C2, criticals);
    if (D2.timedout || D2.early_return)
      return D2;
  }

  if (C1.selected.size() == 1 && is_subset(C1, criticals)) {
    D1 = C1;
  } else {
    D1 = qx_back(sel_union(B, D2.get()), D2.get().selected.size(), C1,
                 criticals);
    if (D1.timedout || D1.early_return)
      return D1;
  }

  return sel_union(D1.get(), D2.get());
}

bool MusEnumerator::qx(Selection &model, const set<MapNode *> &criticals) {
  if (process_native(model))
    return true;

  OptionalSelection res{model};
  do {
    res = qx_back({}, 0, res.get(), criticals);
    if (res.timedout)
      return false;
  } while (res.early_return && !res.is_min);

  model = res.get();
  updateIncludeExclude(model);
  return true;
}

OptionalSelection MusEnumerator::qx2_back(Selection B, size_t D, Selection C,
                                          const set<MapNode *> &criticals) {
  if (mopts.timedOut())
    return QXTimeOut;

  if (D > 0) {
    stats.madeMapCall();
    if (!subsetMap->getSelection(B).selected.empty()) {
      stats.madeSatCheck();
      if (!subProblem.check(B)) {
        stats.foundUnSatSet();

        if (mopts.subproblem_native_shrink) {
          if (process_native(B))
            return QXEarlyMin(B);
          return QXEarlyNonMin(B);
        }

        return empty_sel(C);
      } else {
        stats.foundSatSet();
        subsetMap->blockSubsets(B);
      }
    }
  }
  if (C.selected.size() == 1)
    return C;

  Selection C1, C2;
  // sel_split(mopts, C, C1, C2);
  sel_split(C, C1, C2);

  OptionalSelection D2, D1;
  if (C2.selected.size() == 1 && is_subset(C2, criticals)) {
    D2 = C2;
  } else {
    D2 = qx2_back(sel_union(B, C1), C1.selected.size(), C2, criticals);
    if (D2.timedout || D2.early_return)
      return D2;
  }

  if (C1.selected.size() == 1 && is_subset(C1, criticals)) {
    D1 = C1;
  } else {
    D1 = qx2_back(sel_union(B, D2.get()), D2.get().selected.size(), C1,
                  criticals);
    if (D1.timedout || D1.early_return)
      return D1;
  }

  return sel_union(D1.get(), D2.get());
}

bool MusEnumerator::qx2(Selection &model, const set<MapNode *> &criticals) {
  if (process_native(model))
    return true;

  OptionalSelection res{model};
  do {
    res = qx2_back({}, 0, res.get(), criticals);
    if (res.timedout)
      return false;
  } while (res.early_return && !res.is_min);

  model = res.get();
  updateIncludeExclude(model);
  return true;
}

static int indent = 0;
#define QXLOG(X)                                                               \
  if (mopts.verbose_enum)                                                      \
    std::cout << string(indent * 2, ' ') << "QX: " << X << std::endl;

OptionalSelection
MusEnumerator::qx_back_with_map(Selection B, size_t D, Selection C,
                                const set<MapNode *> &criticals) {
  QXLOG("START");
  indent++;
  if (mopts.timedOut())
    return QXTimeOut;

  if (D > 0 && !B.selected.empty()) {
    stats.madeMapCall();
    if (!subsetMap->getSelection(B).selected.empty()) {
      stats.madeSatCheck();
      if (subProblem.check(B)) {
        stats.foundSatSet();
        subsetMap->blockSubsets(B);
      } else {
        stats.foundUnSatSet();

        if (mopts.subproblem_native_shrink) {
          QXLOG("Returning native unsat set");
          indent--;
          if (process_native(B))
            return QXEarlyMin(B);
          return QXEarlyNonMin(B);
        }

        subsetMap->blockSupersets(B);
        return empty_sel(C);
      }
    }
  }

  if (C.selected.size() == 1) {
    QXLOG("returning C <- |C| == 1");
    indent--;
    return C;
  }

  Selection C1, C2;
  stats.madeMapCall();
  C1 = subsetMap->getRandomSelection(C, B, true);
  if (C1.selected.empty()) {
    QXLOG("returning C <- |C1| == 0");
    indent--;
    return C;
  }
  C2 = sel_complement(C, C1);

  // std::cout << "Part: |C| = " << C.selected.size() << " |C1| = " <<
  // C1.selected.size() << " |C2| = " << C2.selected.size() << std::endl;

  OptionalSelection D2, D1;
  if (C2.selected.size() == 1 && is_subset(C2, criticals)) {
    D2 = C2;
  } else {
    D2 = qx_back_with_map(sel_union(B, C1), C1.selected.size(), C2, criticals);
    if (D2.timedout || D2.early_return)
      return D2;
  }

  if (C1.selected.size() == 1 && is_subset(C1, criticals)) {
    D1 = C1;
  } else {
    D1 = qx_back_with_map(sel_union(B, D2.get()), D2.get().selected.size(), C1,
                          criticals);
    if (D1.timedout || D1.early_return)
      return D1;
  }

  QXLOG("returning D1 U D2");
  indent--;
  return sel_union(D1.get(), D2.get());
}

bool MusEnumerator::qx_with_map(Selection &model,
                                const set<MapNode *> &criticals) {
  if (process_native(model))
    return true;

  OptionalSelection res{model};
  do {
    res = qx_back_with_map({}, 0, res.get(), criticals);
    if (res.timedout)
      return false;
  } while (res.early_return && !res.is_min);

  model = res.get();
  updateIncludeExclude(model);
  return true;
}

MusEnumerator::MusEnumerator(SubProblem &prob, MUSEnumOptions &mo, SubsetMap *m)
    : mopts(mo), subsetMap(m ? m : new MinisatSubsetMap(&prob, mo)),
      subProblem(prob) {}
MusEnumerator::~MusEnumerator() {}

void MusEnumerator::setUnsatCallback(
    std::function<void(const Selection &)> cb) {
  unsat_callback = cb;
}

Statistics &MusEnumerator::getStatistics(void) { return stats; }

void MusEnumerator::printMUS(void) {
  if (current_mus.selected.empty())
    return;
  subProblem.printSol(current_mus);
}

const Selection &MusEnumerator::getCurrentMUS(void) { return current_mus; }

Selection MusEnumerator::getRootSelector(void) {
  return subsetMap->getRootSelector();
}
Selection MusEnumerator::getLeavesSelector(void) {
  return subsetMap->getLeavesSelector();
}

} // namespace HierMUS

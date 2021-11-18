#include <iostream>
#include <set>
#include <string>

#include "Marco.h"

namespace HierMUS {
using std::set;
using std::string;

Marco::Marco(SubProblem &p, MUSEnumOptions &mo, SubsetMap *m)
    : MusEnumerator(p, mo, m) {
  setFrontier(subsetMap->getLeavesSelector());
}

Marco::~Marco() {}

void Marco::setFrontier(const Selection &f) {
  log("New frontier");
  frontier = f;
}

bool Marco::search() {
  if (mopts.timedOut())
    return false;
  while (true) {
    if (mopts.timedOut())
      return false;
    stats.madeMapCall();
    Selection s = subsetMap->getSelection(frontier);

    if (s.size() == 0)
      break;

    stats.madeSatCheck();
    if (!subProblem.check(s)) {
      stats.foundUnSatSet();
      NodeSet empty_crits;
      if (!shrink(s, empty_crits))
        return false;
      frontier.setMinimal(false);
      subsetMap->blockSupersets(s);
      if (s.isLeaves()) {
        current_mus = s;
        return true;
      }
      if (unsat_callback) {
        unsat_callback(s);
        if (mopts.map_enum_focus_mode) {
          // Pretend that we have exhausted this frontier
          return false;
        }
      }
    } else {
      subsetMap->blockSubsets(s);
      stats.foundSatSet();
      if (stats.shouldRestart())
        return false;
    }
  }
  if (frontier.knownMinimal() && frontier.isLeaves()) {
    current_mus = frontier;
    frontier = {};
    frontier.setMinimal(false);
    return true;
  }

  return false;
}

} // namespace HierMUS

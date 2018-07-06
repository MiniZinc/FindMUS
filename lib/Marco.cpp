#include <iostream>
#include <string>
#include <set>

#include "Marco.h"

namespace HierMUS {
  using std::string;
  using std::set;

  Marco::Marco(SubProblem& p, MUSEnumOptions& mo, SubsetMap* m) :
    MusEnumerator(p, mo, m) {
    setFrontier(subsetMap->getLeavesSelector());
  }

  Marco::~Marco() {}

  void Marco::setFrontier(const Selection& f) { 
    if(mopts.verbose_enum) { std::cout << "Marco: new frontier: " << f << "\n"; }
    frontier = f;
  }

  bool Marco::search() {
    if(mopts.timedOut(stats)) return false;
    while(true) {
      if(mopts.timedOut(stats)) return false;
      stats.map_calls++;
      Selection s = subsetMap->getSelection(frontier);

      if(s.selected.size() == 0) break;

      stats.sat_calls++;
      if(!subProblem.check(s)) {
        std::set<MapNode*> empty_crits;
        if(mopts.map_shrink_alg == SH_LIN) {
          shrink(&subProblem, s, empty_crits, stats);
        } else {
          qx(&subProblem, s, empty_crits, stats);
        }
        frontier.is_min = false;
        subsetMap->blockSupersets(s);
        if(isLeaves(s)) {
          current_mus = s;
          return true;
        }
        if(unsat_callback) unsat_callback(s);
      } else {
        subsetMap->blockSubsets(s);
      }
    }
    if(frontier.is_min && isLeaves(frontier)) {
      current_mus = frontier;
      frontier = {};
      frontier.is_min = false;
      return true;
    }

    return false;
  }

}

#include <iostream>

#include "StackMUS.h"

namespace HierMUS {
  StackMUS::StackMUS(SubProblem& prob, MUSEnumOptions& mo, SubsetMap* m) :
    MusEnumerator(prob, mo, m) {
      setFrontier(subsetMap->getLeavesSelector());
    }
  StackMUS::~StackMUS() {}

  void StackMUS::setFrontier(const Selection& f) {
    if(mopts.verbose_enum) { std::cout << "StackMUS: new frontier: " << f << "\n"; }
    shrink_stack = {f};
  }

  bool StackMUS::search() {
    if(mopts.timedOut(stats)) return false;
    while(!shrink_stack.empty()) {
      while(true) {
        if(mopts.timedOut(stats)) return false;
        stats.map_calls++;
        Selection s = subsetMap->getSelection(shrink_stack.back());

        if(s.selected.size() == 0) break;

        stats.sat_calls++;
        if(!subProblem.check(s)) {
          subsetMap->blockSupersets(s);
          if(shrink_stack.size()>0)
            shrink_stack.back().is_min = false;
          shrink_stack.push_back(s);
        } else {
          subsetMap->blockSubsets(s);
        }
      }
      if(shrink_stack.size() > 0) {
        if(shrink_stack.back().is_min) {
          if(isLeaves(shrink_stack.back())) {
            current_mus = shrink_stack.back();
            shrink_stack.pop_back();
            return true;
          }
          if(unsat_callback) {
            unsat_callback(shrink_stack.back());
            if(mopts.map_enum_focus_mode) {
              // Pretend that we have exhausted this frontier;
              return false;
            }
          }
        }
      }
      shrink_stack.pop_back();
    }

    return false;
  }

}

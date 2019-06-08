#include <iostream>
#include <vector>

#include "HierMUSEnumer.h"
#include "StackMUS.h"
#include "Marco.h"
#include "ReMUS.h"

namespace HierMUS {
  using std::vector;

  HierMUSEnumer::HierMUSEnumer(SubProblem& prob, MUSEnumOptions& mo) : MusEnumerator(prob, mo) {
    inner_enum = new Marco(prob, mo, subsetMap);
    inner_enum->setUnsatCallback([&](const Selection& s) {
      if(mopts.verbose_enum) { std::cout << "HierMUSEnumer: SubEnumerator adding set: " << s << " to candidates\n"; }
      if(frontier_idx != -1) {
        if(mopts.verbose_enum) { std::cout << "HierMUSEnumer: Unsetting: " << candidates[frontier_idx] << "\n"; }
        candidates[frontier_idx].is_min = false;
      }
      candidates.push_back(s);
    });

    Selection root = subsetMap->getRootSelector();
    root.is_min = true;
    candidates.push_back(root);
    loadNextCandidate();
  }
  HierMUSEnumer::~HierMUSEnumer() { delete subsetMap; }

  void HierMUSEnumer::setFrontier(const Selection&) { }

  void HierMUSEnumer::loadNextCandidate() {
    Selection top = candidates.back();
    candidates.pop_back();
    Selection expanded = subsetMap->expand(top);
    expanded.is_min = top.is_min;
    if(!isLeaves(expanded)) {
      frontier_idx = static_cast<int>(candidates.size());
      candidates.push_back(expanded);
    } else {
      frontier_idx = -1;
    }
    inner_enum->setFrontier(expanded);
  }

  bool HierMUSEnumer::search() {
    current_mus = empty_selection;
    while(!mopts.timedOut()) {
      if(inner_enum->search()) {
        if(frontier_idx != -1) {
          if(mopts.verbose_enum) { std::cout << "HierMUSEnumer: Unsetting: " << candidates[frontier_idx] << "\n"; }
          candidates[frontier_idx].is_min = false;
        }
        current_mus = inner_enum->getCurrentMUS();
        return true;
      }
      if(candidates.empty())  return false;
      loadNextCandidate();
    }
    return false;
  }

  void HierMUSEnumer::printMUS(void) {
    if(mopts.timedOut()) {
      std::cout << "FindMUS finishing early: ";
      if(candidates.empty()) {
        std::cout << "No remaining candidates.\n";
      } else {
        std::cout << "Last (non-minimal) candidate:\n";
        subProblem.printSol(candidates.back());
      }
      return;
    }
    if(!current_mus.selected.empty()) {
      subProblem.printSol(current_mus);
    }
  }

  Statistics& HierMUSEnumer::getStatistics(void) {
    return inner_enum->getStatistics();
  }

}

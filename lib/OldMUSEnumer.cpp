#include <iostream>
#include <vector>

#include "OldMUSEnumer.h"
#include "StackMUS.h"
#include "Marco.h"
#include "ReMUS.h"

namespace HierMUS {
  using std::vector;

  OldMUSEnumer::OldMUSEnumer(SubProblem& prob, MUSEnumOptions& mo) : MusEnumerator(prob, mo), needs_expansion{false} {
    if(mopts.map_enumeration_alg == ALG_REMUS) {
      inner_enum = new ReMUS(prob, mo, subsetMap);
    } else if(mopts.map_enumeration_alg == ALG_MARCO) {
      inner_enum = new Marco(prob, mo, subsetMap);
    } else {
      inner_enum = new StackMUS(prob, mo, subsetMap);
    }

    inner_enum->setUnsatCallback([&](const Selection& s) {
      if(mopts.verbose_enum) { std::cout << "OldMUSEnumer: SubEnumerator adding set: " << s << " to candidates\n"; }
      frontier = sel_union(frontier, s);
      needs_expansion = !isLeaves(frontier);
    });

    Selection root = subsetMap->getRootSelector();
    frontier = subsetMap->expand(root);
    inner_enum->setFrontier(frontier);
  }
  OldMUSEnumer::~OldMUSEnumer() { delete subsetMap; }

  void OldMUSEnumer::setFrontier(const Selection&) { }

  bool needs_expansion = false;

  bool OldMUSEnumer::search() {
    while(inner_enum->search()) {
      current_mus = inner_enum->getCurrentMUS();
      return true;
    }

    current_mus = empty_selection;
    if(needs_expansion) {
      frontier = subsetMap->expand(frontier);
      inner_enum->setFrontier(frontier);
      subsetMap->reset();
      needs_expansion = false;
      return search();
    }

    return false;
  }

  void OldMUSEnumer::printMUS(void) {
    if(mopts.timedOut()) {
      std::cout << "FindMUS finishing early: ";
        std::cout << "Last frontier (may not be an MUS):\n";
        subProblem.printSol(frontier);
      return;
    }
    if(!current_mus.selected.empty()) {
      subProblem.printSol(current_mus);
    }
  }

  Statistics& OldMUSEnumer::getStatistics(void) {
    return inner_enum->getStatistics();
  }

}

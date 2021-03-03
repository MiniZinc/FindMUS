#include <iostream>
#include <vector>

#include "Marco.h"
#include "OldMUSEnumer.h"
#include "ReMUS.h"

namespace HierMUS {
using std::vector;

void allow_excludes(Selection &s) {
  s.allow_excludes();
}

OldMUSEnumer::OldMUSEnumer(SubProblem &prob, MUSEnumOptions &mo)
    : MusEnumerator(prob, mo), needs_expansion{true} {
  if (mopts.map_enumeration_alg == ALG_REMUS) {
    inner_enum = new ReMUS(prob, mo, subsetMap);
  } else if (mopts.map_enumeration_alg == ALG_MARCO) {
    inner_enum = new Marco(prob, mo, subsetMap);
  }

  inner_enum->setUnsatCallback([&](const Selection &s) {
    if (mopts.verbose_enum) {
      std::cout << "OldMUSEnumer: SubEnumerator adding set: " << s
                << " to frontier, resulting in ";
    }
    frontier = sel_union(frontier, s);
    if (!mopts.map_enum_focus_mode)
      allow_excludes(frontier);
    if (mopts.verbose_enum) {
      std::cout << frontier << "\n";
    }
  });

  Selection root = subsetMap->getRootSelector();
  frontier = subsetMap->expand(root, root);
  inner_enum->setFrontier(frontier);
  needs_expansion = !frontier.isLeaves();
  frontier = empty_selection;
  subsetMap->enableTempBlocking();
}
OldMUSEnumer::~OldMUSEnumer() { delete subsetMap; }

void OldMUSEnumer::setFrontier(const Selection &) {}

bool OldMUSEnumer::search() {
  while (inner_enum->search()) {
    if (!needs_expansion) {
      current_mus = inner_enum->getCurrentMUS();
      return true;
    }
  }

  current_mus = empty_selection;
  if (needs_expansion) {
    frontier = subsetMap->expand(frontier, frontier);
    if (!mopts.map_enum_focus_mode)
      allow_excludes(frontier);

    needs_expansion = !frontier.isLeaves();
    inner_enum->setFrontier(frontier);
    frontier = empty_selection;
    subsetMap->reset();
    return search();
  }

  return false;
}

void OldMUSEnumer::printMUS(void) {
  if (mopts.timedOut()) {
    std::cout << "FindMUS finishing early: ";
    std::cout << "Last frontier (may not be an MUS):\n";
    subProblem.printSol(frontier);
    return;
  }
  if (!current_mus.empty()) {
    subProblem.printSol(current_mus);
  }
}

Statistics &OldMUSEnumer::getStatistics(void) {
  return inner_enum->getStatistics();
}

} // namespace HierMUS

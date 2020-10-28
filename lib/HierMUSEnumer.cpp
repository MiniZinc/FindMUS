#include <iostream>
#include <vector>

#include "HierMUSEnumer.h"
#include "Marco.h"
#include "ReMUS.h"

namespace HierMUS {
using std::vector;

HierMUSEnumer::HierMUSEnumer(SubProblem &prob, MUSEnumOptions &mo)
    : MusEnumerator(prob, mo), frontier_idx{0} {
  if (mo.map_enumeration_alg == ALG_REMUS) {
    if (mo.subproblem_structure != STR_FLAT ||
        mo.subproblem_binarize != BIN_NONE) {
      std::cerr << "Warning: Hierarchical ReMUS has not been implemented. "
                   "Results will be incorrect!"
                << std::endl;
    }
    inner_enum = new ReMUS(prob, mo, subsetMap);
  } else {
    inner_enum = new Marco(prob, mo, subsetMap);
  }

  inner_enum->setUnsatCallback([&](const Selection &s) {
    if (mopts.verbose_enum) {
      std::cout << "HierMUSEnumer: SubEnumerator adding set: " << s
                << " to candidates" << std::endl;
    }
    if (frontier_idx != -1) {
      if (mopts.verbose_enum) {
        std::cout << "HierMUSEnumer: Unsetting: " << candidates[frontier_idx]
                  << std::endl;
      }
      candidates[frontier_idx].is_min = false;
    }
    unsatUnion = sel_union(unsatUnion, s);

    candidates.push_back(subsetMap->expand(s, s));
  });

  Statistics &stats = inner_enum->getStatistics();
  stats.restarts_enabled = mo.restarts_enabled;
  stats.treecounts = subProblem.getTree().getCounts();

  Selection root = subsetMap->getRootSelector();
  root.is_min = true;
  candidates.push_back(root);
  loadNextCandidate();
}
HierMUSEnumer::~HierMUSEnumer() { delete subsetMap; }

void HierMUSEnumer::setFrontier(const Selection &) {}

void HierMUSEnumer::loadNextCandidate() {
  Statistics &stats = inner_enum->getStatistics();
  if (stats.shouldRestart()) {
    if (mopts.verbose_enum) {
      std::cout << "HierMUSEnumer: Restarting in flat mode" << std::endl;
    }
    candidates.clear();
    Selection leaves = subsetMap->getLeavesSelector();
    leaves.is_min = false;
    candidates.push_back(leaves);
    stats.restarts_enabled = false;
  }

  // Expand frontier
  if (frontier_idx != -1) {
    if (unsatUnion.selected.empty()) {
      candidates[frontier_idx] =
          subsetMap->expand(candidates[frontier_idx], candidates[frontier_idx]);
    } else {
      candidates[frontier_idx] =
          subsetMap->expand(candidates[frontier_idx], unsatUnion);
      unsatUnion = {};
    }
  }

  Selection top = candidates.back();
  candidates.pop_back();
  if (!isLeaves(top)) {
    frontier_idx = static_cast<int>(candidates.size());
    candidates.push_back(top);
  } else {
    frontier_idx = -1;
  }
  inner_enum->setFrontier(top);
}

bool HierMUSEnumer::search() {
  current_mus = empty_selection;
  while (!mopts.timedOut()) {
    if (inner_enum->search()) {
      if (frontier_idx != -1) {
        if (mopts.verbose_enum) {
          std::cout << "HierMUSEnumer: Unsetting: " << candidates[frontier_idx]
                    << std::endl;
        }
        candidates[frontier_idx].is_min = false;
      }
      current_mus = inner_enum->getCurrentMUS();
      return true;
    }
    if (candidates.empty())
      return false;
    loadNextCandidate();
  }
  return false;
}

void HierMUSEnumer::printMUS(void) {
  if (mopts.timedOut()) {
    std::cout << "FindMUS finishing early: ";
    if (candidates.empty()) {
      std::cout << "No remaining candidates." << std::endl;
    } else if (mopts.print_leftover) {
      std::cout << "Last (non-minimal) candidate:" << std::endl;
      subProblem.printSol(candidates.back());
    } else {
      std::cout << "Remaining candidate not printed (--no-leftover)"
                << std::endl;
    }
    return;
  }
  if (!current_mus.selected.empty()) {
    subProblem.printSol(current_mus);
  }
}

Statistics &HierMUSEnumer::getStatistics(void) {
  return inner_enum->getStatistics();
}

} // namespace HierMUS

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
      error("Warning: Hierarchical ReMUS has not been implemented. "
            "Results will be incorrect!");
    }
    inner_enum = new ReMUS(prob, mo, subsetMap);
  } else {
    inner_enum = new Marco(prob, mo, subsetMap);
  }

  inner_enum->setUnsatCallback([&](const Selection &s) {
    {
      std::stringstream ss;
      ss << "SubEnumerator adding set: " << s << " to candidates";
      log(ss.str());
    }

    if (frontier_idx != -1) {
      {
        std::stringstream ss;
        ss << "Unsetting: " << candidates[frontier_idx];
        log(ss.str());
      }
      candidates[frontier_idx].setMinimal(false);
    }
    unsatUnion = sel_union(unsatUnion, s);

    candidates.push_back(subsetMap->expand(s, s));
  });

  Statistics &stats = inner_enum->getStatistics();
  stats.restarts_enabled = mo.restarts_enabled;
  stats.treecounts = subProblem.getTree().getCounts();

  Selection root = subsetMap->getRootSelector();
  root.setMinimal(true);
  candidates.push_back(root);
  loadNextCandidate();
}
HierMUSEnumer::~HierMUSEnumer() { delete subsetMap; }

void HierMUSEnumer::setFrontier(const Selection &) {}

void HierMUSEnumer::loadNextCandidate() {
  Statistics &stats = inner_enum->getStatistics();
  if (stats.shouldRestart()) {
    log("Restarting in flat mode");
    candidates.clear();
    Selection leaves = subsetMap->getLeavesSelector();
    leaves.setMinimal(false);
    candidates.push_back(leaves);
    stats.restarts_enabled = false;
  }

  // Expand frontier
  if (frontier_idx != -1) {
    if (unsatUnion.empty()) {
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
  if (!top.isLeaves()) {
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
        {
          std::stringstream ss;
          ss << "Unsetting: " << candidates[frontier_idx];
          log(ss.str());
        }
        candidates[frontier_idx].setMinimal(false);
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
    if (candidates.empty()) {
      log("Finished early. No remaining candidates.");
    } else if (mopts.print_leftover) {
      log("Finished early. Last (non-minimal) candidate:", 0);
      mopts.solution(subProblem.getSolStr(candidates.back()));
    } else {
      log("Finished early. Remaining candidate not printed (--no-leftover)", 0);
    }
    return;
  }
  if (!current_mus.empty()) {
    mopts.solution(subProblem.getSolStr(current_mus));
  }
}

Statistics &HierMUSEnumer::getStatistics(void) {
  return inner_enum->getStatistics();
}

} // namespace HierMUS

#include <algorithm>
#include <iostream>
#include <iterator>

#include "ReMUS.h"

namespace HierMUS {
using std::set;

ReMUS::ReMUS(SubProblem &prob, MUSEnumOptions &mo, SubsetMap *m)
    : MusEnumerator(prob, mo, m) {
  setFrontier(subsetMap->getLeavesSelector());
}

ReMUS::~ReMUS() {}

void ReMUS::setFrontier(const Selection &f) {
  if (mopts.verbose_enum) {
    std::cout << "ReMUS: new frontier: " << f << std::endl;
  }
  remus_stack = {{f, {}}};
  frontier = f;
}

bool ReMUS::search() {
  if (mopts.timedOut())
    return false;
  while (!remus_stack.empty()) {
    if (mopts.timedOut())
      return false;

    // state.S = selected might be anything, include will be accurate, exclude
    // will be accurate.
    ReMUSState &state = remus_stack.back();
    const int depth = static_cast<int>(remus_stack.size()) - 1;
    const string indent(depth, '\t');
    if (mopts.verbose_enum) {
      std::cout << indent << "ReMUS: stack.size(): " << remus_stack.size()
                << " top: " << state.S << " crits: {" << state.criticals << " }"
                << std::endl;
    }
    stats.map_calls++;

    // s.selected and s.include are equal and s.exclude is accurate.
    Selection s_max = subsetMap->getSelection(state.S);

    if (s_max.selected.size() == 0) {
      if (mopts.verbose_enum) {
        std::cout << indent << "ReMUS: popping" << std::endl;
      }
      remus_stack.pop_back();
      continue;
    }

    stats.sat_calls++;
    if (!subProblem.check(s_max)) {
      frontier.is_min = false;
      Selection s_mus = s_max; // Copy of s with maximal s
      if (!shrink(s_mus, state.criticals))
        return false;
      subsetMap->blockSupersets(s_mus);
      subsetMap->blockSubsets(s_mus);
      if (s_mus.selected.size() < floor(0.9 * s_max.selected.size())) {
        Selection p = s_mus; // s_mus subset of p subset of s_max
        p.exclude.insert(s_max.selected.begin(), s_max.selected.end());

        set<MapNode *>::iterator it = s_max.selected.begin();
        while (it != s_max.selected.end() &&
               p.selected.size() < floor(0.9 * s_max.selected.size())) {
          p.selected.insert(*it);
          p.include.insert(ExpandedNode(*it));
          p.exclude.erase(*it);
          ++it;
        }
        updateIncludeExclude(p);

        if (mopts.verbose_enum) {
          std::cout << indent << "ReMUS: pushing: " << p
                    << " crits: " << state.criticals << std::endl;
        }
        remus_stack.push_back({p, state.criticals});
      }
      if (isLeaves(s_mus)) {
        current_mus = s_mus;
        return true;
      }
      if (unsat_callback) {
        s_mus.is_min = true;
        unsat_callback(s_mus);
        if (mopts.map_enum_focus_mode) {
          // Pretend we have exhausted this frontier
          return false;
        }
      }
    } else {
      subsetMap->blockSubsets(s_max);
      set<ExpandedNode> s_mcs;
      std::set_difference(state.S.include.begin(), state.S.include.end(),
                          s_max.include.begin(), s_max.include.end(),
                          std::inserter(s_mcs, s_mcs.begin()));
      if (s_mcs.size() == 1) {
        state.criticals.insert((*s_mcs.begin()).child);
        if (mopts.verbose_enum) {
          std::cout << indent << "ReMUS: updated crits: {" << state.criticals
                    << " }" << std::endl;
        }
      } else {
        // We are aboute to modify remus_stack so we need a copy of criticals
        set<MapNode *> criticals_copy = state.criticals;
        set<ExpandedNode>::reverse_iterator rit;
        for (rit = s_mcs.rbegin(); rit != s_mcs.rend(); ++rit) {
          MapNode *n = (*rit).child;
          ReMUSState new_state{s_max, criticals_copy};
          new_state.S.selected.insert(n);
          new_state.S.include.insert(ExpandedNode(n));
          new_state.S.exclude.erase(n);
          new_state.criticals.insert(n);
          if (mopts.verbose_enum) {
            std::cout << indent << "ReMUS: pushing: " << new_state.S
                      << " crits: {" << new_state.criticals << " }"
                      << std::endl;
          }
          remus_stack.push_back(new_state);
        }
      }
    }
  }
  if (frontier.is_min && isLeaves(frontier)) {
    current_mus = frontier;
    frontier = {};
    frontier.is_min = false;
    return true;
  }

  return false;
}

} // namespace HierMUS

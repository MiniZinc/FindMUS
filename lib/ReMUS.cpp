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
  std::stringstream ss;
  ss  << "ReMUS: new frontier: " << f;
  log(ss.str());

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

    {
      std::stringstream ss;
      ss << indent << "ReMUS: stack.size(): " << remus_stack.size()
         << " top: " << state.S << " crits: {" << state.criticals << " }";
      log(ss.str());
    }
    stats.map_calls++;

    // s.selected and s.include are equal and s.exclude is accurate.
    Selection s_max = subsetMap->getSelection(state.S);

    if (s_max.size() == 0) {
      log("ReMUS: popping");
      remus_stack.pop_back();
      continue;
    }

    stats.sat_calls++;
    if (!subProblem.check(s_max)) {
      frontier.setMinimal(false);
      Selection s_mus = s_max; // Copy of s with maximal s
      if (!shrink(s_mus, state.criticals))
        return false;
      subsetMap->blockSupersets(s_mus);
      subsetMap->blockSubsets(s_mus);
      // if s_mus is 10% smaller than s_max
      if (s_mus.size() < floor(0.9 * s_max.size())) {
        // copy s_mus into p (it is smaller and should be consistent)
        Selection p = s_mus; // s_mus subset of p subset of s_max
        Selection missing = s_max.get_diff(p);

        // compute how many elements should be added
        size_t n_remove = floor(0.9 * s_max.size() - s_mus.size());
        auto it = missing.const_selected().rbegin();
        while(n_remove--) {
          p.select(*(it++));
        }

        std::stringstream ss;
        ss << indent << "ReMUS: pushing: " << p << " crits: " << state.criticals;
        log(ss.str());

        remus_stack.push_back({p, state.criticals});
      }
      if (s_mus.isLeaves()) {
        current_mus = s_mus;
        return true;
      }
      if (unsat_callback) {
        s_mus.setMinimal(true);
        unsat_callback(s_mus);
        if (mopts.map_enum_focus_mode) {
          // Pretend we have exhausted this frontier
          return false;
        }
      }
    } else {
      subsetMap->blockSubsets(s_max);
      Selection s_mcs = state.S.get_diff(s_max);

      if (s_mcs.size() == 1) {
        state.criticals.insert((*s_mcs.const_included().begin()).child);

        std::stringstream ss;
        ss << indent << "ReMUS: updated crits: {" << state.criticals << " }";
        log(ss.str());
      } else {
        // We are aboute to modify remus_stack so we need a copy of criticals
        NodeSet criticals_copy = state.criticals;
        const NodeSet& s_mcs_included = s_mcs.const_included().get_nodes();
        vector<const MapNode *>::const_reverse_iterator rit;
        for (rit = s_mcs_included.rbegin(); rit != s_mcs_included.rend(); ++rit) {
          const MapNode *n = *rit;
          ReMUSState new_state{s_max, criticals_copy};
          new_state.S.select(n);
          new_state.criticals.insert(n);

          std::stringstream ss;
          ss << indent << "ReMUS: pushing: " << new_state.S << " crits: {" << new_state.criticals << " }";
          log(ss.str());

          remus_stack.push_back(new_state);
        }
      }
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

#ifndef __HIERMUS_SUBSETMAP_H_
#define __HIERMUS_SUBSETMAP_H_

#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>

#include "Types.h"
#include "SubProblem.h"
#include "Options.h"

namespace HierMUS {

  struct conj_disj {
    int idx : 31;
    bool is_branch : 1;
  };

  class SubsetMap {
    protected:
      MUSEnumOptions& mopts;
      SubProblem* problem;
    public:
      SubsetMap(SubProblem* prob, MUSEnumOptions& mo) : mopts(mo), problem(prob) {}
      virtual ~SubsetMap() {}
      virtual Selection expand(const Selection& selection) = 0;
      virtual Selection getSelection(const Selection& selection) = 0;
      virtual Selection getSelection() = 0;
      virtual Selection getRootSelector() = 0;
      virtual Selection getLeavesSelector() = 0;
      virtual void blockSupersets(const Selection& selection) = 0;
      virtual void blockSubsets(const Selection& selection) = 0;
  };
}

#endif

#ifndef __HIERMUS_SUBSETMAP_H_
#define __HIERMUS_SUBSETMAP_H_

#include <set>

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
      std::set<MapNode*> forceInclude;

    public:
      SubsetMap(SubProblem* prob, MUSEnumOptions& mo) : mopts(mo), problem(prob) {}
      virtual ~SubsetMap() {}

      void setForceInclude(const Selection& selection);
      void clearForceInclude(void);

      virtual void pushTemporarySubsetBlock(const Selection& selection) = 0;
      virtual void popTemporarySubsetBlock(void) = 0;

      virtual Selection expand(const Selection& selection) = 0;
      virtual void setMaximal(bool max_mode) = 0;
      virtual Selection getRandomSelection(const Selection& selection, const Selection& finc);
      virtual Selection getSelection(const Selection& selection) = 0;
      virtual Selection getSelection() = 0;
      virtual Selection getRootSelector() = 0;
      virtual Selection getLeavesSelector() = 0;
      virtual void blockSupersets(const Selection& selection) = 0;
      virtual void blockSubsets(const Selection& selection, bool weak_block = true) = 0;
  };
}

#endif

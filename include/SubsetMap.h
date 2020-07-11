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

      bool consistent;
      bool tempBlocking;

    public:
      SubsetMap(SubProblem* prob, MUSEnumOptions& mo) : mopts(mo), problem(prob), consistent{true}, tempBlocking{false} {}
      virtual ~SubsetMap() {}

      void setForceInclude(const Selection& selection);
      void clearForceInclude(void);

      void enableTempBlocking(void) { tempBlocking = true; }
      void disableTempBlocking(void) { tempBlocking = false; }

      virtual void pushTempBlockSupersets(const Selection& selection) = 0;
      virtual void pushTempBlockSubsets(const Selection& selection) = 0;
      virtual void popTempBlock(void) = 0;
      virtual void reset(void) = 0;

      virtual Selection expand(const Selection& selection, const Selection& target) = 0;
      virtual void setMaximal(bool max_mode) = 0;

      virtual Selection getRandomSelection(const Selection& selection, const Selection& finc, bool strictSubset);
      virtual Selection getSelection(const Selection& selection) = 0;

      virtual Selection getSelection() = 0;
      virtual Selection getRootSelector() = 0;
      virtual Selection getLeavesSelector() = 0;
      virtual void blockSupersets(const Selection& selection) = 0;
      virtual void blockSubsets(const Selection& selection) = 0;

      virtual Selection convertConIds(std::set<string>& con_ids) = 0;

      bool isConsistent(void) const { return consistent; }
  };
}

#endif

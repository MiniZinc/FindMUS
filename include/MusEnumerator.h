#ifndef __HIERMUS_MUSENUMERATOR_H_
#define __HIERMUS_MUSENUMERATOR_H_

#include <string>
#include <vector>
#include <set>
#include <functional>

#include "Types.h"
#include "SubProblem.h"
#include "SubsetMap.h"

namespace HierMUS {
  class MusEnumerator {
    protected:
      MUSEnumOptions& mopts;

      SubsetMap* subsetMap;
      SubProblem& subProblem;
      Statistics stats;
      Selection current_mus;
      std::function<void(const Selection&)> unsat_callback;

    public:
      MusEnumerator(SubProblem& p, MUSEnumOptions& mo, SubsetMap* m = NULL);
      virtual ~MusEnumerator();

      virtual void setFrontier(const Selection& f) = 0;
      virtual bool search() = 0;
      virtual void setUnsatCallback(std::function<void(const Selection&)> cb);
      virtual void printMUS();
      virtual Statistics& getStatistics();
      virtual const Selection& getCurrentMUS();
      Selection getRootSelector();
      Selection getLeavesSelector();

      static void updateIncludeExclude(Selection& s);
      static bool shrink(MUSEnumOptions& mopts, SubProblem* prob, Selection& model,
                         const std::set<MapNode*>& criticals, Statistics& stats);
      static bool qx(MUSEnumOptions& mopts, SubProblem* prob, Selection& model,
                     const std::set<MapNode*>& criticals, Statistics& stats);
  };
}

#endif

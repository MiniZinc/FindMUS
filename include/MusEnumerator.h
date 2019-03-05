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

  struct OptionalSelection {
    Selection v;
    bool has_v;

    OptionalSelection(void) : has_v{false} {}

    OptionalSelection(const OptionalSelection& o) {
      has_v = o.has_value();
      if(has_v) v = o.v;
    }
    OptionalSelection(const Selection& s) {
      has_v = true;
      v = s;
    }

    OptionalSelection& operator=(const OptionalSelection& o) {
      v = o.v;
      has_v = o.has_v;
      return *this;
    }

    Selection& get(void) { return v; }
    bool has_value(void) const { return has_v; }
  };

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
      bool shrink(Selection& model, const std::set<MapNode*>& criticals);

    private:
      bool linear_shrink(Selection& model, const std::set<MapNode*>& criticals);
      bool linear_shrink_with_map(Selection& model, const std::set<MapNode*>& criticals);
      bool qx(Selection& model, const std::set<MapNode*>& criticals);
      bool qx_with_map(Selection& model, const std::set<MapNode*>& criticals);

      OptionalSelection qx_back(Selection B, Selection D, Selection C,
                                const std::set<MapNode*>& criticals);
      OptionalSelection qx_back_with_map(Selection B, int D, Selection C,
                                         const std::set<MapNode*>& criticals);
  };
}

#endif

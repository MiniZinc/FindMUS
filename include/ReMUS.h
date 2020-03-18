#ifndef __HIERMUS_REMUS_H_
#define __HIERMUS_REMUS_H_

#include <vector>
#include <set>

#include "MusEnumerator.h"

namespace HierMUS {
  class ReMUS : public MusEnumerator {
    private:
      struct ReMUSState {
        Selection S;
        std::set<MapNode*> criticals;
      };

      std::vector<ReMUSState> remus_stack;
      Selection frontier;

    public:
      ReMUS(SubProblem& p, MUSEnumOptions& mo, SubsetMap* m = NULL);
      ~ReMUS();

      void setFrontier(const Selection& f);
      bool search();
  };
}

#endif


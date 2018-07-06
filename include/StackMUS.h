#ifndef __HIERMUS_STACKMUS_H_
#define __HIERMUS_STACKMUS_H_

#include <vector>

#include "MusEnumerator.h"

namespace HierMUS {
  class StackMUS : public MusEnumerator {
    private:
      std::vector<Selection> shrink_stack;

    public:
      StackMUS(SubProblem& p, MUSEnumOptions& mo, SubsetMap* m = NULL);
      ~StackMUS();

      void setFrontier(const Selection& f);
      bool search();
  };
}

#endif


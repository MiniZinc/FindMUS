#ifndef __HIERMUS_MARCO_H_
#define __HIERMUS_MARCO_H_

#include "MusEnumerator.h"

namespace HierMUS {
  class Marco : public MusEnumerator {
    private:
      Selection selection;
      Selection frontier;

    public:
      Marco(SubProblem& p, MUSEnumOptions& mo, SubsetMap* m = NULL);
      ~Marco();

      void setFrontier(const Selection& f);
      bool search();
  };
}

#endif


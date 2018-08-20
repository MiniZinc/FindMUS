#ifndef __HIERMUS_HIERMUS_H_
#define __HIERMUS_HIERMUS_H_

#include <vector>

#include "MusEnumerator.h"

namespace HierMUS {
  class HierMUSEnumer : public MusEnumerator {
    private:
      std::vector<Selection> candidates;
      MusEnumerator* inner_enum;

      void loadNextCandidate(void);
      int frontier_idx;

    public:
      HierMUSEnumer(SubProblem& p, MUSEnumOptions& mo);
      ~HierMUSEnumer();

      void setFrontier(const Selection& f);
      bool search(void);
      void printMUS(void);
      Statistics& getStatistics(void);
  };
}

#endif


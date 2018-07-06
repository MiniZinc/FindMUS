#ifndef _HIERMUS_DEMOPROBLEMS_H_
#define _HIERMUS_DEMOPROBLEMS_H_

#include <vector>
#include <string>

#include "SubProblem.h"
#include "Types.h"

namespace HierMUS {

  class HM5 : public SubProblem {
    public:
      explicit HM5(MUSEnumOptions& mo);
      void printSol(const Selection& b);
      bool check(const Selection& b);
  };
  
  class HM5_2 : public SubProblem {
    public:
      explicit HM5_2(MUSEnumOptions& mo);
      void printSol(const Selection& b);
      bool check(const Selection& b);
  };

  class FFLAT : public SubProblem {
    public:
      explicit FFLAT(MUSEnumOptions& mo);
      void printSol(const Selection& b);
      bool check(const Selection& b);
  };

}

#endif

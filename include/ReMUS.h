#ifndef __HIERMUS_REMUS_H_
#define __HIERMUS_REMUS_H_

#include <set>
#include <vector>

#include "MusEnumerator.h"

namespace HierMUS {
class ReMUS : public MusEnumerator {
private:
  struct ReMUSState {
    Selection S;
    NodeSet criticals;
  };

  std::vector<ReMUSState> remus_stack;
  Selection frontier;

public:
  ReMUS(SubProblem &p, MUSEnumOptions &mo, SubsetMap *m = NULL);
  ~ReMUS();

  void setFrontier(const Selection &f);
  bool search();
};
} // namespace HierMUS

#endif

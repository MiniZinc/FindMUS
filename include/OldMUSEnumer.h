#ifndef __HIERMUS_OLDMUSENUMER_H_
#define __HIERMUS_OLDMUSENUMER_H_

#include <vector>

#include "MusEnumerator.h"

namespace HierMUS {
class OldMUSEnumer : public MusEnumerator {
private:
  Selection frontier;
  MusEnumerator *inner_enum;
  bool needs_expansion;

public:
  OldMUSEnumer(SubProblem &p, MUSEnumOptions &mo);
  ~OldMUSEnumer();

  void setFrontier(const Selection &f);
  bool search(void);
  void printMUS(void);
  Statistics &getStatistics(void);
};
} // namespace HierMUS

#endif

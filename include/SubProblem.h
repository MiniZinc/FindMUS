#ifndef __HIERMUS_PROBLEM_H_
#define __HIERMUS_PROBLEM_H_

#include <vector>
#include <string>
#include <set>

#include "Types.h"
#include "Options.h"

namespace HierMUS {

  class SubsetMap;


  class SubProblem {
    protected:
      MapNode tree;
      MUSEnumOptions& mopts;
    public:
      explicit SubProblem(MUSEnumOptions& mo);
      virtual ~SubProblem();
      virtual void printSol(const Selection& b) = 0;
      virtual bool check(const Selection& b) = 0;
      virtual bool provedSAT() = 0;
      virtual MapNode& getTree();

      std::vector<std::string> leaf_names;
  };

}

#endif

#ifndef _HIERMUS_FILESP_H_
#define _HIERMUS_FILESP_H_

#include <vector>
#include <set>
#include <string>

#include "SubProblem.h"
#include "Types.h"

namespace HierMUS {

  class FileSP: public SubProblem {
    public:
      explicit FileSP(MUSEnumOptions& mo, std::string file_path);
      void printSol(const Selection& b);
      bool check(const Selection& b);
      bool provedSAT() { return false; } ;

    private:
      std::vector<std::set<std::string>> muses;
  };

}

#endif

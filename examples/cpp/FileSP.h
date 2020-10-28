#ifndef _HIERMUS_FILESP_H_
#define _HIERMUS_FILESP_H_

#include <set>
#include <string>
#include <vector>

#include "SetTrie.h"
#include "SubProblem.h"
#include "Types.h"

namespace HierMUS {

class FileSP : public SubProblem {
public:
  explicit FileSP(MUSEnumOptions &mo, std::istream &is);
  explicit FileSP(MUSEnumOptions &mo, std::string file_path);
  void init(std::istream &is);
  void printSol(const Selection &b);
  bool check(const Selection &b);
  bool provedSAT() { return false; };

  // These are public for tests
  SetTrie oracle;
  std::vector<std::set<std::string>> muses;
  bool isMUS(const Selection &b);
};

} // namespace HierMUS

#endif

#ifndef __HIERMUS_PROBLEM_H_
#define __HIERMUS_PROBLEM_H_

#include <string>
#include <vector>

#include "Options.h"
#include "Types.h"

namespace HierMUS {

class SubsetMap;

class SubProblem {
protected:
  MapNode tree;
  MUSEnumOptions &mopts;

public:
  explicit SubProblem(MUSEnumOptions &mo);
  virtual ~SubProblem();
  virtual std::string getSolStr(const Selection &b) = 0;
  virtual bool check(const Selection &b) = 0;
  virtual ShrunkSet getShrunk() { return {}; }
  virtual bool provedSAT() = 0;
  virtual MapNode &getTree();

  std::vector<std::string> leaf_names;

  void log(const std::string& msg, unsigned int level = 1) const;
  void error(const std::string& msg) const;
};

} // namespace HierMUS

#endif

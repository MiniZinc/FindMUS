#ifndef _HIERMUS_DEMOPROBLEMS_H_
#define _HIERMUS_DEMOPROBLEMS_H_

#include <string>
#include <vector>

#include "SubProblem.h"
#include "Types.h"

namespace HierMUS {

class HM5 : public SubProblem {
public:
  explicit HM5(MUSEnumOptions &mo);
  std::string getSolStr(const Selection &b);
  bool check(const Selection &b);
  bool provedSAT() { return false; };
};

class GLM : public SubProblem {
public:
  explicit GLM(MUSEnumOptions &mo);
  std::string getSolStr(const Selection &b);
  bool check(const Selection &b);
  bool provedSAT() { return false; };
};

class HM5_2 : public SubProblem {
public:
  explicit HM5_2(MUSEnumOptions &mo);
  std::string getSolStr(const Selection &b);
  bool check(const Selection &b);
  bool provedSAT() { return false; };
};

class FFLAT : public SubProblem {
public:
  explicit FFLAT(MUSEnumOptions &mo);
  std::string getSolStr(const Selection &b);
  bool check(const Selection &b);
  bool provedSAT() { return false; };
};

class Path : public SubProblem {
public:
  explicit Path(MUSEnumOptions &mo);
  std::string getSolStr(const Selection &b);
  bool check(const Selection &b);
  bool provedSAT() { return false; };
};

class P1f : public SubProblem {
public:
  explicit P1f(MUSEnumOptions &mo);
  std::string getSolStr(const Selection &b);
  bool check(const Selection &b);
  bool provedSAT() { return false; };
};

class RandomProblem : public SubProblem {
private:
  std::set<std::set<string>> muses;

public:
  explicit RandomProblem(MUSEnumOptions &mo, int seed, unsigned int ncons,
                         unsigned int nmuses, unsigned int mussize);
  std::string getSolStr(const Selection &b);
  bool check(const Selection &b);
  bool provedSAT() { return false; };
};

} // namespace HierMUS

#endif

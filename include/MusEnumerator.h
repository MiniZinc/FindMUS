#ifndef __HIERMUS_MUSENUMERATOR_H_
#define __HIERMUS_MUSENUMERATOR_H_

#include <functional>
#include <set>
#include <string>
#include <vector>

#include "SubProblem.h"
#include "SubsetMap.h"
#include "Types.h"

namespace HierMUS {

struct OptionalSelection {
  Selection v;

  bool timedout;
  bool early_return;
  bool is_min;

  OptionalSelection(void)
      : timedout{true}, early_return{false}, is_min{false} {}

  OptionalSelection(const OptionalSelection &o) {
    timedout = o.timedout;
    early_return = o.early_return;
    is_min = o.is_min;
    v = o.v;
  }

  OptionalSelection(const Selection &s)
      : v{s}, timedout{false}, early_return{false}, is_min{false} {}

  OptionalSelection(const Selection &s, bool early, bool min)
      : v{s}, timedout{false}, early_return{early}, is_min{min} {}

  OptionalSelection &operator=(const OptionalSelection &o) {
    v = o.v;
    timedout = o.timedout;
    early_return = o.early_return;
    is_min = o.is_min;
    return *this;
  }

  Selection &get(void) { return v; }
};

class MusEnumerator {
protected:
  MUSEnumOptions &mopts;

  SubsetMap *subsetMap;
  SubProblem &subProblem;
  Statistics stats;
  Selection current_mus;
  std::function<void(const Selection &)> unsat_callback;

  void log(const std::string& msg, unsigned int level = 1) const;
  void error(const std::string& msg) const;

public:
  MusEnumerator(SubProblem &p, MUSEnumOptions &mo, SubsetMap *m = NULL);
  virtual ~MusEnumerator();

  virtual void setFrontier(const Selection &f) = 0;
  virtual bool search() = 0;
  virtual void setUnsatCallback(std::function<void(const Selection &)> cb);
  virtual void printMUS();
  virtual Statistics &getStatistics();
  virtual const Selection &getCurrentMUS();
  Selection getRootSelector();
  Selection getLeavesSelector();

  static void updateIncludeExclude(Selection &s);
  bool shrink(Selection &model, const NodeSet &criticals);

private:
  bool process_native(Selection &model);

  bool native_shrink(Selection &model, const NodeSet &criticals);

  bool linear_shrink(Selection &model, const NodeSet &criticals);
  bool linear_shrink_with_map(Selection &model,
                              const NodeSet &criticals);
  bool qx(Selection &model, const NodeSet &criticals);
  bool qx2(Selection &model, const NodeSet &criticals);
  bool qx_with_map(Selection &model, const NodeSet &criticals);

  OptionalSelection qx_back(Selection B, size_t D, Selection C,
                            const NodeSet &criticals);
  OptionalSelection qx2_back(Selection B, size_t D, Selection C,
                             const NodeSet &criticals);
  OptionalSelection qx_back_with_map(Selection B, size_t D, Selection C,
                                     const NodeSet &criticals);

};
} // namespace HierMUS

#endif

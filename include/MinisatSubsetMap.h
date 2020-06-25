#ifndef __HIERMUS_MINISAT_SUBSETMAP_H_
#define __HIERMUS_MINISAT_SUBSETMAP_H_

#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>

#include "Types.h"
#include "SubProblem.h"
#include "SubsetMap.h"
#include "Options.h"

namespace HierMUS {

  class MinisatSubsetProblem : public SubsetMap, public Problem {
    private:
      MapNode addConnections(const MapNode& node, unsigned int depth = 0);

    public:
      MinisatSubsetProblem(SubProblem* prob, MUSEnumOptions& mo);
      ~MinisatSubsetProblem() {  }

      void pushTempBlockSupersets(const Selection& selection);
      void pushTempBlockSubsets(const Selection& selection);
      void popTempBlock(void);
      void reset(void);

      void print(std::ostream&);
      void setMaximal(bool max_mode);
      Selection expand(const Selection& selection, const Selection& range);
      Selection getSelection(const Selection& selection, bool blockSat = true);
      Selection getSelection();
      Selection getRootSelector();
      Selection getLeavesSelector();

      void block(vec<Lit>& blockClause);
      void blockSupersets(const Selection& selection);
      void blockSubsets(const Selection& selection);
  };

}

#endif

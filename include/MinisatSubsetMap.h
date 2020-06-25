#ifndef __HIERMUS_MINISAT_SUBSETMAP_H_
#define __HIERMUS_MINISAT_SUBSETMAP_H_

#include <minisat/core/Solver.h>

#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>

#include "Types.h"
#include "SubProblem.h"
#include "SubsetMap.h"
#include "Options.h"

namespace HierMUS {

  class MinisatSubsetMap : public SubsetMap {
    private:
      Minisat::vec<Minisat::Var> leaves;
      Minisat::vec<Minisat::Var> conjs;
      Minisat::vec<Minisat::Var> disjs;
      Minisat::vec<Minisat::Var> eqs;

      std::vector<MapNode> leafNodes;
      std::vector<MapNode> branchNodes;

      std::vector<Minisat::Lit> tempStack;

      MapNode root;

      Minisat::Solver solver;

      int leaves_top;
      int branches_top;

      Selection solution_set;
      Selection solution_template;

    private:
      MapNode addConnections(const MapNode& node, unsigned int depth = 0);

    public:
      MinisatSubsetMap(SubProblem* prob, MUSEnumOptions& mo);
      ~MinisatSubsetMap() { }

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

      void block(Minisat::vec<Minisat::Lit>& blockClause);
      void blockSupersets(const Selection& selection);
      void blockSubsets(const Selection& selection);
  };

}

#endif

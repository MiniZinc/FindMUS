#ifndef __HIERMUS_CHUFFED_SUBSETMAP_H_
#define __HIERMUS_CHUFFED_SUBSETMAP_H_

#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>

#include "Types.h"
#include "SubProblem.h"
#include "SubsetMap.h"
#include "Options.h"

namespace HierMUS {

  class ChuffedSubsetProblem : public SubsetMap, public Problem {
    private:
      vec<BoolView> leaves;
      vec<BoolView> conjs;
      vec<BoolView> disjs;
      vec<BoolView> eqs;

      std::vector<MapNode> leafNodes;
      std::vector<MapNode> branchNodes;

      std::vector<Lit> tempStack;

      MapNode root;

      vec<Branching*> branching_vars;

      int leaves_top;
      int branches_top;

      IntVar* obj;

      Selection solution_set;
      Selection solution_template;

      std::fstream null_stream;

    private:
      MapNode addConnections(const MapNode& node, unsigned int depth = 0);

    public:
      ChuffedSubsetProblem(SubProblem* prob, MUSEnumOptions& mo);
      ~ChuffedSubsetProblem() { delete obj; }

      void pushTempBlockSupersets(const Selection& selection);
      void pushTempBlockSubsets(const Selection& selection);
      void popTempBlock(void);
      void reset(void);

      void print(std::ostream&);
      void setMaximal(bool max_mode);
      Selection expand(const Selection& selection);
      Selection getSelection(const Selection& selection);
      Selection getSelection();
      Selection getRootSelector();
      Selection getLeavesSelector();

      void block(vec<Lit>& blockClause);
      void blockSupersets(const Selection& selection);
      void blockSubsets(const Selection& selection);
  };

}

#endif

#ifndef __HIERMUS_SUBSETMAP_H_
#define __HIERMUS_SUBSETMAP_H_

#include <vector>
#include <memory>
#include <unordered_map>

#include "Types.h"
#include "SubProblem.h"
#include "Options.h"

namespace HierMUS {

  struct conj_disj {
    int idx : 31;
    bool is_branch : 1;
  };

  class SubsetMap {
    protected:
      MUSEnumOptions& mopts;
      SubProblem* problem;
    public:
      SubsetMap(SubProblem* prob, MUSEnumOptions& mo) : mopts(mo), problem(prob) {}
      virtual ~SubsetMap() {}
      virtual Selection expand(const Selection& selection) = 0;
      virtual Selection getSelection(const Selection& selection) = 0;
      virtual Selection getSelection() = 0;
      virtual Selection getRootSelector() = 0;
      virtual Selection getLeavesSelector() = 0;
      // virtual void getImplied(lbool dir, Assignment& implied) const = 0;
      virtual void blockSupersets(const Selection& selection) = 0;
      virtual void blockSubsets(const Selection& selection) = 0;
      // virtual void setAssignment(Assignment& as) const = 0;
  };

  class ChuffedSubsetProblem : public SubsetMap, public Problem {
    private:
      vec<BoolView> leaves;
      vec<BoolView> conjs;
      vec<BoolView> disjs;
      vec<BoolView> eqs;

      std::vector<MapNode> leafNodes;
      std::vector<MapNode> branchNodes;

      MapNode root;

      vec<Branching*> branching_vars;

      int leaves_top;
      int branches_top;

      IntVar* obj;

      Selection solution_set;
      Selection solution_template;

      bool consistent;

    private:
      MapNode addConnections(const MapNode& node, int depth = 0);
      void addObjective();

    public:
      ChuffedSubsetProblem(SubProblem* prob, MUSEnumOptions& mo);
      ~ChuffedSubsetProblem() { delete obj; }

      void print(std::ostream&);
      Selection expand(const Selection& selection);
      Selection getSelection(const Selection& selection);
      Selection getSelection();
      Selection getRootSelector();
      Selection getLeavesSelector();

      // void setAssignment(Assignment& as) const ;

      // void getImplied(lbool dir, Assignment& implied) const;
      void block(const Selection& selection, bool polarity);
      void blockSupersets(const Selection& selection);
      void blockSubsets(const Selection& selection);
  };


}

#endif

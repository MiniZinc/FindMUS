#ifndef __HIERMUS_FZNPROBLEM_H_
#define __HIERMUS_FZNPROBLEM_H_

#include "SubProblem.h"
#include "NamePathMap.h"
#include "Types.h"
#include "path_utils.h"

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

#include <minizinc/model.hh>
#include <minizinc/solver.hh>
#include <minizinc/solns2out.hh>

namespace HierMUS {
  
  class NullSolns2Out : public MiniZinc::Solns2Out {
    public:
      std::ofstream nullstream;
      NullSolns2Out();
      virtual ~NullSolns2Out();
      virtual std::ostream& getOutput();
  };

  class FznSubProblem : public SubProblem {
    private:
      std::ofstream nullstream;
      std::stringstream log;

      MiniZinc::Env fzn_env;
      NullSolns2Out s2o;
      MiniZinc::Model* fzn_model;

      bool last_sat;
      ShrunkSet shrunk;

      std::unordered_map<std::string, MiniZinc::ConstraintI*> constraints;
      NamePathMap nameToPath;
      std::unordered_map<int, std::string> solverModelMapping;
      std::string fzn_file;

      ConstraintSet getConstraintSet(const Selection& b);
      bool isBackgroundConstraint(const MiniZinc::ConstraintI& ci, const string& name);
      bool isFilteredIn(const MiniZinc::ConstraintI& ci, const string& name);
      void saveFzn(const Selection& b, const string& filename);

    public:
      FznSubProblem(const std::string& fznpath, const std::string& pathpath, MUSEnumOptions& mo);
      ~FznSubProblem() {
        delete fzn_model;
      }

      void printSol(const Selection& b);
      void printLongSol(const Selection& b);
      void printHtml(const Selection& b);

      bool check(const Selection& b);

      void setShrunk(const std::vector<int>&, bool min);
      ShrunkSet getShrunk();

      bool provedSAT();
  };
}

#endif

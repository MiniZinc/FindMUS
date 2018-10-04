#ifndef __HIERMUS_FZNPROBLEM_H_
#define __HIERMUS_FZNPROBLEM_H_

#include "SubProblem.h"
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

      std::unordered_map<std::string, MiniZinc::ConstraintI*> constraints;
      std::unordered_map<std::string, std::string> nameToPath;
      std::string fzn_file;

      ConstraintSet getConstraintSet(const std::set<std::string>& names);
      bool isBackgroundConstraint(const MiniZinc::ConstraintI& ci, const string& name);
      void saveFzn(const Selection& b, const string& filename);

    public:
      FznSubProblem(std::string fznpath, std::string pathfilepath, MUSEnumOptions& mo);
      ~FznSubProblem() {
        delete fzn_model;
      }

      void printSol(const Selection& b);
      void printLongSol(const Selection& b);
      void printHtml(const Selection& b);
      bool check(const Selection& b);
  };
}

#endif

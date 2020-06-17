#ifndef __HIERMUS_FZNPROBLEM_H_
#define __HIERMUS_FZNPROBLEM_H_

#include "SetTrie.h"
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

      NullSolns2Out s2o;

      MiniZinc::Env fzn_env;
      MiniZinc::Model* fzn_model;

      SetTrie oracle;

      bool last_sat;

      std::unordered_map<std::string, MiniZinc::ConstraintI*> constraints;
      NamePathMap nameToPath;

      void init_fzn_model(const string& fznpath);
      void init_oracle_model(const string& oraclepath, int ncons, vector<string>& background_cons);
      ConstraintSet getConstraintSet(const Selection& b);
      bool isBackgroundConstraint(const MiniZinc::ConstraintI& ci, const string& name);
      void saveFzn(const Selection& b, const string& filename);

    public:
      FznSubProblem(const std::string& fznpath, const std::string& pathpath, MUSEnumOptions& mo, const std::string& oraclepath);
      ~FznSubProblem() { delete fzn_model; }

      void printSol(const Selection& b);
      void printLongSol(const Selection& b);
      void printHtml(const Selection& b);
      bool check(const Selection& b);
      bool provedSAT();
  };
}

#endif

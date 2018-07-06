#ifndef __HIERMUS_FZNPROBLEM_H_
#define __HIERMUS_FZNPROBLEM_H_

#include "SubProblem.h"
#include "Types.h"

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <regex>

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

  struct NaA {
    std::string assigns;
    std::string name;
    std::string explain;
  };

  class FznSubProblem : public SubProblem {
    private:
      std::ofstream nullstream;
      std::stringstream log;

      MiniZinc::Env fzn_env;
      NullSolns2Out s2o;
      MiniZinc::Model* fzn_model;
      MiniZinc::SolverFactory* solver_factory = 0;
      MiniZinc::SolverInstanceBase::Options* solver_options = 0;

      std::unordered_map<std::string, MiniZinc::ConstraintI*> constraints;
      std::unordered_map<std::string, std::string> nameToPath;
      std::string fzn_file;

      static std::regex assignment_regex;
      static std::regex generalize_regex;
      vector<string> getAllAssigns(const string& path) const;

      std::unordered_map<std::string, std::vector<NaA> > getEntries(std::set<std::string>& names);
      static std::string generalizeLabel(std::string& path_el, bool mix);
      bool isBackgroundConstraint(const MiniZinc::ConstraintI& ci, const string& name);

    public:
      FznSubProblem(std::string fznpath, std::string pathfilepath, MUSEnumOptions& mo);
      ~FznSubProblem() {
        delete fzn_model;
        delete solver_options;
        delete solver_factory;
      }

      void printSol(const Selection& b);
      void printLongSol(const Selection& b);
      std::string getShortSol(const Selection& b, const std::string& sep = " ", bool esc = false);
      void printHtml(const Selection& b);
      bool check(const Selection& b);
  };
}

#endif

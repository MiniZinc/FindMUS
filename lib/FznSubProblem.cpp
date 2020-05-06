#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifdef _MSC_VER
#include <io.h>
#define NULL_PATH "nul"
#define dup _dup
#define dup2 _dup2
#else // POSIX
#define NULL_PATH "/dev/null"
#endif

#include <minizinc/file_utils.hh>
#include <minizinc/solver.hh>

#include "FznSubProblem.h"
#include "Types.h"
#include "string_utils.h"
#include "path_utils.h"

#ifdef _MSC_VER
#undef ERROR
#endif


static int saved_stdout;
static int saved_stderr;
static int null_file;


inline void silence_output_start() {
  saved_stdout = dup(STDOUT_FILENO);
  saved_stderr = dup(STDOUT_FILENO);
  null_file = open(NULL_PATH, O_WRONLY, 0600);

  dup2(null_file, STDOUT_FILENO);
  dup2(null_file, STDERR_FILENO);
}

inline void silence_output_end() {
  dup2(saved_stdout, STDOUT_FILENO);
  dup2(saved_stderr, STDERR_FILENO);
  close(null_file);
}

namespace HierMUS {
  using std::string;
  using std::vector;
  using std::stringstream;
  using std::ifstream;
  using std::ofstream;
  using std::set;
  using std::unordered_map;

  NullSolns2Out::NullSolns2Out() : Solns2Out(nullstream, nullstream, "") {}
  NullSolns2Out::~NullSolns2Out() {}
  std::ostream& NullSolns2Out::getOutput() { return nullstream; }

  bool FznSubProblem::isBackgroundConstraint(const MiniZinc::ConstraintI& ci, const string& name) {
    if(nameToPath.at(name) == "NOPATH") return true;
    if(mopts.subproblem_hard_functional_constraints &&
       ci.e()->ann().containsCall(MiniZinc::constants().ann.defines_var))
       return true;
    if(mopts.subproblem_hard_domain_constraints &&
       ci.e()->ann().contains(MiniZinc::constants().ann.domain_change_constraint))
       return true;
    static const vector<string> ann_strs = {"mzn_expression_name", "mzn_constraint_name"};
    if(mopts.subproblem_named_only) {
      for(const string& ann_str : ann_strs) {
        MiniZinc::Expression* e = MiniZinc::getAnnotation(ci.e()->ann(), ann_str);
        if(e) return false;
      }
      return true;
    } else if(!mopts.subproblem_name_filters.empty() ||
              !mopts.subproblem_name_filters_excludes.empty()) {
      for(const string& ann_str : ann_strs) {
        MiniZinc::Expression* e = MiniZinc::getAnnotation(ci.e()->ann(), ann_str);
        if(e) {
          string name = e->cast<MiniZinc::Call>()->arg(0)->cast<MiniZinc::StringLit>()->v().str();
          for(const string& exclude_string: mopts.subproblem_name_filters_excludes) {
            if(name.find(exclude_string) != string::npos) return true;
          }
        }
      }
      for(const string& ann_str : ann_strs) {
        MiniZinc::Expression* e = MiniZinc::getAnnotation(ci.e()->ann(), ann_str);
        if(e) {
          string name = e->cast<MiniZinc::Call>()->arg(0)->cast<MiniZinc::StringLit>()->v().str();
          for(const string& exclude_string: mopts.subproblem_name_filters) {
            if(name.find(exclude_string) != string::npos) return false;
          }
        }
      }
      return !mopts.subproblem_name_filters.empty();
    } else if(!mopts.subproblem_path_filters.empty() ||
              !mopts.subproblem_path_filters_excludes.empty()) {
      const string& path = nameToPath.at(name);
      // excludes
      for(const string& exfil : mopts.subproblem_path_filters_excludes) {
        if(path.find(exfil) != string::npos) return true;
      }
      // includes
      for(const string& fil : mopts.subproblem_path_filters) {
        if(path.find(fil) != string::npos) return false;
      }
      return !mopts.subproblem_path_filters.empty();
    } else {
      return false;
    }
  }

  FznSubProblem::FznSubProblem(
      string fznpath, string pathfilepath,
      MUSEnumOptions& mo) : SubProblem(mo), last_sat{false}, has_shrunk{false} {
    double start_build = wallClockTime();
    ifstream pathstream(pathfilepath);
    if(!pathstream.is_open()) {
      std::cerr << "FznSubproblem:\tCan't open pathfile: " << pathfilepath << "\n";
      exit(EXIT_FAILURE);
    }
    string line;
    while(std::getline(pathstream, line)) {
      vector<string> entry = utils::split(line, '\t', true);
      if(isdigit(entry[0][0])) {
        string leaf_name = entry[0];
        leaf_names.push_back(leaf_name);
        if(entry.size()==3)
          nameToPath[leaf_name] = entry[2];
        else
          nameToPath[leaf_name] = "NOPATH";
      }
    }

    MiniZinc::GCLock lock;
    vector<string> includes;

    includes.push_back(mo.mzn_stdlib_dir + "/std/");

    fzn_model = MiniZinc::parse(fzn_env, {fznpath}, {}, "", "", includes, false, false, false, std::cerr);
    MiniZinc::SolveI* si = fzn_model->solveItem();
    si->st(MiniZinc::SolveI::ST_SAT);
    si->e(nullptr);
    fzn_env.model(fzn_model);

    vector<MiniZinc::TypeError> typeErrors;
    try {
      MiniZinc::typecheck(fzn_env, fzn_model, typeErrors, true, true, true);
    } catch (MiniZinc::TypeError& e) {
      typeErrors.push_back(e);
    }
    if(typeErrors.size() > 0) {
      for(unsigned int i=0; i<typeErrors.size(); i++) {
        std::cerr << typeErrors[i].loc() << ":" << std::endl;
        std::cerr << typeErrors[i].what() << ":" << typeErrors[i].msg() << std::endl;
      }
      exit(EXIT_FAILURE);
    }

    MiniZinc::registerBuiltins(fzn_env);
    fzn_env.swap();
    MiniZinc::populateOutput(fzn_env);
    fzn_env.model(fzn_model);
    s2o.initFromEnv(&fzn_env);

    for(auto it = fzn_model->begin(); it != fzn_model->end(); ++it) {
      //if((*it)->isa<MiniZinc::IncludeI>() || (*it)->isa<MiniZinc::FunctionI>()) { (*it)->remove(); }
      if((*it)->isa<MiniZinc::IncludeI>()) { (*it)->remove(); }
      if((*it)->isa<MiniZinc::VarDeclI>()) {
        MiniZinc::VarDeclI* vdi = (*it)->cast<MiniZinc::VarDeclI>();
        MiniZinc::VarDecl* vd = vdi->e();
        MiniZinc::Annotation& ann = vd->ann();
        ann.removeCall(MiniZinc::constants().ann.output_array);
        ann.remove(MiniZinc::constants().ann.output_var);
      }
    }
    fzn_model->compact();

    size_t con_id = 0;
    unsigned int hard_cons = 0;
    unsigned int soft_cons = 0;

    for(auto cit = fzn_model->begin_constraints(); cit != fzn_model->end_constraints(); ++cit) {
      MiniZinc::ConstraintI& ci = *cit;
      string name = leaf_names[con_id];
      if(isBackgroundConstraint(ci, name)) {
        hard_cons++;
      } else {
        if(leaf_names.size() <= con_id) {
          std::cerr << "FznSubProblem:\tError: Path file does not match FlatZinc\n";
          exit(EXIT_FAILURE);
        }
        constraints[name] = &ci;
        if(mopts.subproblem_structure == STR_FLAT) {
          tree.children.push_back(MapNode(nameToPath[name], name));
        } else {
          string path;
          if(mopts.subproblem_structure != STR_NORMAL) { // Not STR_FLAT and not STR_NORMAL
            path = utils::generalizeLabel(nameToPath[name], mopts.subproblem_structure == STR_IDX_MIX || mopts.subproblem_structure == STR_IDX,
                                                            mopts.subproblem_structure == STR_GEN_MIX || mopts.subproblem_structure == STR_IDX_MIX);
          } else {
            path = nameToPath[name];
          }
          MapNode& last = tree.addPath(path);
          stringstream ss;
          vector<string> cons = utils::split(last.con_id, '#');
          cons.push_back(name);
          last.con_id = utils::join(cons, "#");
        }
        soft_cons++;
      }
      con_id++;
    }


    counts cs = tree.getCounts();
    int nbranches;
    do {
      nbranches = cs.nbranches;
      tree.compact();
      cs = tree.getCounts();
    } while(nbranches != cs.nbranches);

    if(mopts.subproblem_structure == STR_GEN)
      tree.mergeLeaves();

    if(mopts.subproblem_binarize == BIN_ALL) {
      tree.makeBinary([](const MapNode& n){ 
          return n.children.size() > 2;
          });
    }

    cs = tree.getCounts();
    //if(mopts.verbose_subsolve) {
    std::cout << "FznSubProblem:\thard cons: " << hard_cons << "\tsoft cons: " << soft_cons << "\tleaves: " << cs.nleaves << "\tbranches: " << cs.nbranches << "\tBuilt tree in "
      << std::fixed << std::setprecision(5) << wallClockTime() - start_build
      << " seconds.\n";
    //}
  }

  bool FznSubProblem::provedSAT() { return last_sat; }

  ConstraintSet FznSubProblem::getConstraintSet(const Selection& b) {
    set<string> leaf_names = getLeaves(b);
    ConstraintSet entries;
    for(const string& leaf_name : leaf_names) {
      string path = nameToPath[leaf_name];
      ConstraintInfo n;
      n.leaf_name = leaf_name;
      n.setAnnotatedNamesFrom(constraints[leaf_name]);

      vector<string> split_all = utils::split(path, MINOR_SEP);
      n.name = split_all.back();

      if(mopts.map_depth == DEPTH_INSTANCE) {
        vector<string> splitPath = utils::getPathHead(path, false, true);
        path = utils::join(splitPath, string(1, MAJOR_SEP));
      } else if(mopts.map_depth == DEPTH_CUSTOM) {
        vector<string> splitPath = utils::split(path, MAJOR_SEP);
        splitPath.resize(mopts.map_depth_max);
        path = utils::join(splitPath, string(1, MAJOR_SEP));
      }

      n.path = path;
      n.assigns = utils::join(utils::getAllAssigns(path), ",");
      entries.addConstraintInfo(path, n);
    }
    return entries;
  }

  inline
  std::set<string> FznSubProblem::getShrunk() {
    assert(hasShrunk());
    return shrunk;
  }

  void FznSubProblem::setShrunk(const std::vector<int>& solver_mus) {
    has_shrunk = true;
    shrunk.clear();
    for(int idx : solver_mus) {
      shrunk.insert(leaf_names[idx]);
    }
  }

  bool FznSubProblem::check(const Selection& b) {
    double beginCheck = wallClockTime();
    has_shrunk = false;
    set<string> leaves = getLeaves(b);
    // Mark all constraints as removed;
    for(auto& kv : constraints) { kv.second->remove(); }
    // Activate the selected constraints
    for(const string& l : leaves) { constraints[l]->unremove(); }

    MiniZinc::MznSolver solver(nullstream, log);

    // Build arguments for MznSolver
    vector<string> args;
    args.push_back("minizinc"); // Make sure MznSolver knows to run in minizinc driver mode
    args.push_back("--solver");
    args.push_back(mopts.subproblem_solver);

    mopts.adjustSolverTimeout();
    args.push_back("--solver-time-limit");
    args.push_back(std::to_string(mopts.subproblem_solver_time_limit));
    vector<string> split_extra_args = utils::split(mopts.subproblem_solver_flags, ' ');
    args.insert(args.end(), split_extra_args.begin(), split_extra_args.end());

    std::vector<string> args_vec(args);
    switch (solver.processOptions(args_vec)) {
      case 0:
        break;
      default:
        std::cerr << "FznSubProblem:\tError creating solver with args:\t" << utils::join(args, " ") << std::endl;
        std::cerr << "\t" << log.str() << "\n";
        exit(EXIT_FAILURE);
    }

    silence_output_start();

    MiniZinc::SolverFactory* sf = solver.getSF();
    MiniZinc::SolverInstanceBase* si = sf->createSI(fzn_env, log, solver.getSI_OPT());
    si->setSolns2Out(&s2o);
    si->processFlatZinc();


    MiniZinc::SolverInstance::Status s = MiniZinc::SolverInstance::ERROR;
    try {
      s = si->solve();
    } catch (const MiniZinc::InternalError& err) {
      std::cerr << "FznSubproblem:\tException during sub-solving: "
                << err.msg() << ": " << err.what() << std::endl;
      exit(EXIT_FAILURE);
    }

    silence_output_end();

    std::string error_log = log.str();
    if(!error_log.empty()) {
      std::cerr << "FznSubproblem:\tstderr from MznSolver:\n" << error_log << "\n";
      log.clear();
      exit(EXIT_FAILURE);
    }

    string res;
    bool is_sat = s != MiniZinc::SolverInstance::UNSAT;
    last_sat = false;
    if (s == MiniZinc::SolverInstance::SAT || s == MiniZinc::SolverInstance::OPT) {
      last_sat = true;
      res = "S";
    } else if (s == MiniZinc::SolverInstance::UNSAT) {
      res = "U";

      if(si->hasMUS()) {
        setShrunk(si->getMUS());
      }

    } else if (s == MiniZinc::SolverInstance::ERROR) {
      res = "E";
      string errfilename = "FINDMUS_failed_subproblem.fzn";
      std::cout << "FznSubproblem:\tSolver reported error without message\n";
      saveFzn(b, errfilename);
    } else {
      res = "?";
    }

    if(mopts.verbose_subsolve) {
      std::cout << "FznSubProblem:\tSolve: " << s << "(" << res << ":" << (is_sat ? "S" : "U") << ") ";
      if(mopts.verbose_subsolve > 1) {
        std::cout << "cons: " << b;
      } else {
        std::cout << "ncons: " << std::setw(8) << leaves.size();
      }
      std::cout << "\ttook: " << std::fixed << std::setprecision(5) << (wallClockTime() - beginCheck) << " seconds" << std::endl;
    }

    sf->destroySI(si);
    return is_sat;
  }

  void FznSubProblem::saveFzn(const Selection& b, const string& filename) {
    std::cout << "FznSubProblem:\tdumping fzn as: " << filename << "\n";
    set<string> leaves = getLeaves(b);
    // Mark all constraints as removed;
    for(auto& kv : constraints) { kv.second->remove(); }
    // Activate the selected constraints
    for(const string& l : leaves) { constraints[l]->unremove(); }

    std::ofstream f;
    f.open(filename);
    if(f.is_open()) {
      MiniZinc::Printer p(f, 0, true);
      for (MiniZinc::FunctionIterator it = fzn_model->begin_functions(); it != fzn_model->end_functions(); ++it) {
        if(!it->removed()) {
          MiniZinc::Item& item = *it;
          p.print(&item);
        }
      }
      for (MiniZinc::VarDeclIterator it = fzn_model->begin_vardecls(); it != fzn_model->end_vardecls(); ++it) {
        if(!it->removed()) {
          MiniZinc::Item& item = *it;
          p.print(&item);
        }
      }
      for (MiniZinc::ConstraintIterator it = fzn_model->begin_constraints(); it != fzn_model->end_constraints(); ++it) {
        if(!it->removed()) {
          MiniZinc::Item& item = *it;
          p.print(&item);
        }
      }
      p.print(fzn_model->solveItem());
      f.close();
    } else {
      std::cout << "cannot open file" << filename << " for writing\n";
    }
  }

  void FznSubProblem::printSol(const Selection& b) {
    ConstraintSet cs = getConstraintSet(b);
    std::cout << cs.getSummary(mopts.subproblem_output_format);
  }
}

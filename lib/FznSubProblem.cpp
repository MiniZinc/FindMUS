#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <limits>

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

inline void silence_init() {
  saved_stdout = dup(STDOUT_FILENO);
  saved_stderr = dup(STDERR_FILENO);
  null_file = open(NULL_PATH, O_WRONLY, 0600);
}

inline void silence_output_start() {
  dup2(null_file, STDOUT_FILENO);
  dup2(null_file, STDERR_FILENO);
}

inline void silence_output_end() {
  dup2(saved_stdout, STDOUT_FILENO);
  dup2(saved_stderr, STDERR_FILENO);
}

namespace HierMUS {
  using std::string;
  using std::vector;
  using std::pair;
  using std::stringstream;
  using std::ifstream;
  using std::ofstream;
  using std::set;
  using std::unordered_map;

  NullSolns2Out::NullSolns2Out() : Solns2Out(nullstream, nullstream, "") {}
  NullSolns2Out::~NullSolns2Out() {}
  std::ostream& NullSolns2Out::getOutput() { return nullstream; }

  inline bool isFunctionalConstraint(const MiniZinc::ConstraintI& ci) {
    return ci.e()->ann().containsCall(MiniZinc::constants().ann.defines_var);
  }
  inline bool isDomainConstraint(const MiniZinc::ConstraintI& ci) {
    return ci.e()->ann().contains(MiniZinc::constants().ann.domain_change_constraint);
  }

  bool FznSubProblem::isBackgroundConstraint(const MiniZinc::ConstraintI& ci, const string& name) {
    return nameToPath.getPath(name) == "NOPATH" ||
           (mopts.subproblem_hard_functional_constraints &&
            isFunctionalConstraint(ci)) ||
           (mopts.subproblem_hard_domain_constraints &&
            isDomainConstraint(ci));
  }

  struct ConNames {
    string cons_name;
    string expr_name;
  };

  inline ConNames getNames(const MiniZinc::ConstraintI& ci) {
    ConNames names;
    MiniZinc::Expression* cn = MiniZinc::getAnnotation(ci.e()->ann(), "mzn_constraint_name");
    if(cn) names.cons_name = cn->cast<MiniZinc::Call>()->arg(0)->cast<MiniZinc::StringLit>()->v().str();
    MiniZinc::Expression* en = MiniZinc::getAnnotation(ci.e()->ann(), "mzn_expression_name");
    if(en) names.expr_name = en->cast<MiniZinc::Call>()->arg(0)->cast<MiniZinc::StringLit>()->v().str();
    return names;
  }

  bool FznSubProblem::isFilteredIn(const MiniZinc::ConstraintI& ci, const string& name) {
    ConNames names = getNames(ci);

    if(!mopts.subproblem_name_filters.empty() && mopts.subproblem_name_filters[0] == "*") {
      return !(names.cons_name.empty() && names.expr_name.empty());
    }

    if(!mopts.subproblem_name_filters.empty() || !mopts.subproblem_name_filters_excludes.empty()) {
      for(const string& exclude_string: mopts.subproblem_name_filters_excludes) {
        if(names.cons_name.find(exclude_string) != string::npos ||
           names.expr_name.find(exclude_string) != string::npos)
          return false;
      }
      for(const string& include_string: mopts.subproblem_name_filters) {
        if(names.cons_name.find(include_string) != string::npos ||
           names.expr_name.find(include_string) != string::npos)
          return true;
      }
      return mopts.subproblem_name_filters.empty();
    }

    if(!mopts.subproblem_path_filters.empty() || !mopts.subproblem_path_filters_excludes.empty()) {
      const string& path = nameToPath.getPath(name);
      // excludes
      for(const string& exfil : mopts.subproblem_path_filters_excludes) {
        if(path.find(exfil) != string::npos) return false;
      }
      // includes
      for(const string& fil : mopts.subproblem_path_filters) {
        if(path.find(fil) != string::npos) return true;
      }
      return mopts.subproblem_path_filters.empty();
    }

    return true;
  }

  FznSubProblem::FznSubProblem(
      const string& fznpath, const string& pathpath,
      MUSEnumOptions& mo) : SubProblem(mo), last_sat{false}, shrunk{}, nameToPath{pathpath}, fzn_file (fznpath) {
    silence_init();

    double start_build = wallClockTime();

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

    size_t n_cons = nameToPath.hasNCons() ? nameToPath.getNCons() : std::numeric_limits<size_t>::max();
    size_t con_id = 0;
    unsigned int hard_cons = 0;
    unsigned int ignored_cons = 0;
    unsigned int soft_cons = 0;

    for(auto cit = fzn_model->begin_constraints(); cit != fzn_model->end_constraints(); ++cit) {
      MiniZinc::ConstraintI& ci = *cit;
      string name = nameToPath.getName(con_id);
      bool background = isBackgroundConstraint(ci, name);
      if(isBackgroundConstraint(ci, name)) {
        hard_cons++;
      } else if(isFilteredIn(ci, name)) {
        if(n_cons <= con_id) {
          std::cerr << "FznSubProblem:\tError: Path file does not match FlatZinc\n";
          exit(EXIT_FAILURE);
        }
        constraints[name] = &ci;
        if(mopts.subproblem_structure == STR_FLAT) {
          tree.children.push_back(MapNode(nameToPath.getPath(name), name));
        } else {
          string path;
          if(mopts.subproblem_structure != STR_NORMAL) { // Not STR_FLAT and not STR_NORMAL
            path = utils::generalizeLabel(nameToPath.getPath(name), mopts.subproblem_structure == STR_IDX_MIX || mopts.subproblem_structure == STR_IDX,
                                                            mopts.subproblem_structure == STR_GEN_MIX || mopts.subproblem_structure == STR_IDX_MIX);
          } else {
            path = nameToPath.getPath(name);
          }
          MapNode& last = tree.addPath(path);
          stringstream ss;
          vector<string> cons = utils::split(last.con_id, '#');
          cons.push_back(name);
          last.con_id = utils::join(cons, "#");
        }
        soft_cons++;
      } else { // Not background, not include by filter
        if(mopts.subproblem_filter_mode == FILTER_FOREGROUND) {
          hard_cons++;
        } else { // FILTER_EXCLUSIVE
          ignored_cons++;
          ci.remove();
        }
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
    std::cout << "FznSubProblem:\thard cons: " << hard_cons << "\tsoft cons: " << soft_cons << "\tleaves: " << cs.nleaves << "\tbranches: " << cs.nbranches << "\tignored cons: " << ignored_cons << "\tBuilt tree in "
      << std::fixed << std::setprecision(5) << wallClockTime() - start_build
      << " seconds.\n";
    //}
  }

  bool FznSubProblem::provedSAT() { return last_sat; }

  ConstraintSet FznSubProblem::getConstraintSet(const Selection& b) {
    set<string> leaf_names = getLeaves(b);
    ConstraintSet entries;
    for(const string& leaf_name : leaf_names) {
      string path = nameToPath.getPath(leaf_name);
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
  ShrunkSet FznSubProblem::getShrunk() {
    assert(!shrunk.con_ids.empty());
    return shrunk;
  }

  void FznSubProblem::setShrunk(const std::vector<int>& solver_mus, bool min) {
    shrunk.minimal = min;
    shrunk.con_ids.clear();
    for(int idx : solver_mus) {
      auto it = solverModelMapping.find(idx);
      if(it != solverModelMapping.end()) {
        shrunk.con_ids.insert(it->second);
      }
    }
  }

  bool FznSubProblem::check(const Selection& b) {
    double beginCheck = wallClockTime();

    shrunk.minimal = false;
    shrunk.con_ids.clear();

    // Mark all constraints as removed;
    for(auto& kv : constraints) { 
      kv.second->remove();
    }

    int solver_con_id = 0;
    int con_id = 0;
    // Activate the selected constraints
    set<string> leaves = getLeaves(b);

    solverModelMapping.clear();

    for(size_t i=0; i<fzn_model->size(); i++) {
      if(MiniZinc::ConstraintI* ci = (*fzn_model)[i]->dyn_cast<MiniZinc::ConstraintI>()) {
        if(!ci->removed()) {
          // Constraint is part of background
          solver_con_id++;
        } else {
          // Constraint is part of foreground but might not be added
          string con_id_s = std::to_string(con_id);
          if(leaves.find(con_id_s) != leaves.end()) {
            ci->unremove();
            solverModelMapping[solver_con_id] = con_id_s;
            solver_con_id++;
          }
        }
        con_id++;
      }
    }

    // for(const string& l : leaves) { constraints[l]->unremove(); }

    MiniZinc::MznSolver solver(nullstream, log);

    // Build arguments for MznSolver
    vector<string> args;
    args.push_back("minizinc"); // Make sure MznSolver knows to run in minizinc driver mode
    args.push_back("--solver");
    args.push_back(mopts.subproblem_solver);

    if(mopts.subproblem_native_shrink) {
      args.push_back("--diagnose");
    }

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
      silence_output_end();
    } catch (const MiniZinc::InternalError& err) {
      silence_output_end();
      std::cerr << "FznSubproblem:\tException during sub-solving: "
                << err.msg() << ": " << err.what() << std::endl;
      exit(EXIT_FAILURE);
    } catch (...) {
      silence_output_end();
      std::cerr << "FznSubproblem:\tCaught unknown exception during sub-solving\n";
      exit(EXIT_FAILURE);
    }

    std::string error_log = log.str();
    if(!error_log.empty()) {
      std::cerr << "FznSubproblem:\tstderr from MznSolver:\n" << error_log << "\n";
      log.clear();
      exit(EXIT_FAILURE);
    }

    static int unsat_c = 0;
    string res;
    bool is_sat = s != MiniZinc::SolverInstance::UNSAT;
    last_sat = false;
    if (s == MiniZinc::SolverInstance::SAT || s == MiniZinc::SolverInstance::OPT) {
      last_sat = true;
      res = "S";
    } else if (s == MiniZinc::SolverInstance::UNSAT) {
      res = "U";

      auto statusMUS = si->getMUSStatus();

      if(statusMUS != MiniZinc::SolverInstance::MUS_NONE) {
        setShrunk(si->getMUS(), statusMUS == MiniZinc::SolverInstance::MUS_MINIMAL);
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
      std::cout << "\ttook: " << std::fixed << std::setprecision(5) << (wallClockTime() - beginCheck) << " seconds";
      std::cout << "\tmus: " << "(" << !shrunk.con_ids.empty() << ","  << shrunk.minimal << ")" << std::endl;

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

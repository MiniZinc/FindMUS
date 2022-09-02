#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#include <minizinc/file_utils.hh>
#include <minizinc/solver.hh>

#include "FznSubProblem.h"
#include "Types.h"
#include "path_utils.h"
#include "string_utils.h"

#ifdef _MSC_VER
#undef ERROR
#endif

namespace HierMUS {
using std::ifstream;
using std::ofstream;
using std::pair;
using std::set;
using std::string;
using std::stringstream;
using std::unordered_map;
using std::vector;

NullSolns2Out::NullSolns2Out(string stdlib_dir)
    : Solns2Out(nullstream, nullstream, stdlib_dir) {}
NullSolns2Out::~NullSolns2Out() {}
std::ostream &NullSolns2Out::getOutput() { return nullstream; }

inline bool isFunctionalConstraint(const MiniZinc::ConstraintI &ci) {
  return ci.e()->ann().containsCall(MiniZinc::Constants::constants().ann.defines_var);
}
inline bool isDomainConstraint(const MiniZinc::ConstraintI &ci) {
  return ci.e()->ann().contains(
      MiniZinc::Constants::constants().ann.domain_change_constraint);
}

bool FznSubProblem::isBackgroundConstraint(const MiniZinc::ConstraintI &ci,
                                           const string &name) {
  return nameToPath.getPath(name) == "NOPATH" ||
         (mopts.subproblem_hard_functional_constraints &&
          isFunctionalConstraint(ci)) ||
         (mopts.subproblem_hard_domain_constraints && isDomainConstraint(ci));
}

struct ConNames {
  string cons_name;
  string expr_name;
};

inline ConNames getNames(const MiniZinc::ConstraintI &ci) {
  ConNames names;
  MiniZinc::Expression *cn =
      MiniZinc::get_annotation(ci.e()->ann(), "mzn_constraint_name");
  if (cn)
    names.cons_name = cn->cast<MiniZinc::Call>()
                          ->arg(0)
                          ->cast<MiniZinc::StringLit>()
                          ->v()
                          .c_str();
  MiniZinc::Expression *en =
      MiniZinc::get_annotation(ci.e()->ann(), "mzn_expression_name");
  if (en)
    names.expr_name = en->cast<MiniZinc::Call>()
                          ->arg(0)
                          ->cast<MiniZinc::StringLit>()
                          ->v()
                          .c_str();
  return names;
}

bool FznSubProblem::isFilteredIn(const MiniZinc::ConstraintI &ci,
                                 const string &name) {
  ConNames names = getNames(ci);

  if (!mopts.subproblem_name_filters.empty() ||
      !mopts.subproblem_name_filters_excludes.empty()) {
    for (const string &exclude_string :
         mopts.subproblem_name_filters_excludes) {
      if (names.cons_name.find(exclude_string) != string::npos ||
          names.expr_name.find(exclude_string) != string::npos)
        return false;
    }
    if (!mopts.subproblem_name_filters.empty() &&
        mopts.subproblem_name_filters[0] == "*") {
      return !(names.cons_name.empty() && names.expr_name.empty());
    }
    for (const string &include_string : mopts.subproblem_name_filters) {
      if (names.cons_name.find(include_string) != string::npos ||
          names.expr_name.find(include_string) != string::npos)
        return true;
    }
    return mopts.subproblem_name_filters.empty();
  }

  if (!mopts.subproblem_path_filters.empty() ||
      !mopts.subproblem_path_filters_excludes.empty()) {
    const string &path = nameToPath.getPath(name);
    // excludes
    for (const string &exfil : mopts.subproblem_path_filters_excludes) {
      if (path.find(exfil) != string::npos)
        return false;
    }
    // includes
    for (const string &fil : mopts.subproblem_path_filters) {
      if (path.find(fil) != string::npos)
        return true;
    }
    return mopts.subproblem_path_filters.empty();
  }

  return true;
}

void FznSubProblem::init_oracle_model(const string &oraclepath, int ncons,
                                      vector<string> &background_cons) {
  ifstream oracle_log(oraclepath);
  if (!oracle_log.is_open())
    return;

  string line;
  while (std::getline(oracle_log, line)) {
    if (line.size() > 4 && line.substr(0, 4) == "MUS:") {
      vector<string> cons = utils::split(line.substr(5, line.size()), ' ');
      oracle.add_set(cons);
    }
  }
}

void FznSubProblem::init_fzn_model(const string &fznpath) {
  MiniZinc::GCLock lock;
  vector<string> includes;

  includes.push_back(mopts.mzn_stdlib_dir + "/std/");

  std::stringstream mzn_err;
  fzn_model = MiniZinc::parse(fzn_env, {fznpath}, {}, "", "", includes, {}, false,
                              false, false, false, mzn_err);
  std::string errs = mzn_err.str();
  if(!errs.empty()) {
    error(errs);
  }

  MiniZinc::SolveI *si = fzn_model->solveItem();
  si->st(MiniZinc::SolveI::ST_SAT);
  si->e(nullptr);
  fzn_env.model(fzn_model);

  vector<MiniZinc::TypeError> typeErrors;
  try {
    MiniZinc::typecheck(fzn_env, fzn_model, typeErrors, true, true, true);
  } catch (MiniZinc::TypeError &e) {
    typeErrors.push_back(e);
  }
  if (typeErrors.size() > 0) {
    for (unsigned int i = 0; i < typeErrors.size(); i++) {
      std::stringstream ss;
      ss << typeErrors[i].loc() << ":" << std::endl
         << typeErrors[i].what() << ":" << typeErrors[i].msg() << std::endl;
      error(ss.str());
    }
    exit(EXIT_FAILURE);
  }

  MiniZinc::register_builtins(fzn_env);
  fzn_env.swap();
  MiniZinc::populate_output(fzn_env, false);
  fzn_env.model(fzn_model);
  s2o.initFromEnv(&fzn_env);
}

FznSubProblem::FznSubProblem(const string &fznpath, const string &pathpath,
                             MUSEnumOptions &mo, const string &oraclepath = "")
    : SubProblem(mo), s2o(mo.mzn_stdlib_dir), fzn_model{nullptr},
      oracle{"root", false}, last_sat{false}, shrunk{}, nameToPath{pathpath} {

  auto start_build = std::chrono::system_clock::now();

  init_fzn_model(fznpath);

  for (auto it = fzn_model->begin(); it != fzn_model->end(); ++it) {
    // if((*it)->isa<MiniZinc::IncludeI>() || (*it)->isa<MiniZinc::FunctionI>())
    // { (*it)->remove(); }
    if ((*it)->isa<MiniZinc::IncludeI>()) {
      (*it)->remove();
    }
    if ((*it)->isa<MiniZinc::VarDeclI>()) {
      MiniZinc::VarDeclI *vdi = (*it)->cast<MiniZinc::VarDeclI>();
      MiniZinc::VarDecl *vd = vdi->e();
      MiniZinc::Annotation &ann = vd->ann();
      ann.removeCall(MiniZinc::Constants::constants().ann.output_array);
      ann.remove(MiniZinc::Constants::constants().ann.output_var);
    }
  }
  fzn_model->compact();

  size_t n_cons = nameToPath.hasNCons() ? nameToPath.getNCons()
                                        : std::numeric_limits<size_t>::max();
  size_t con_id = 0;
  unsigned int hard_cons = 0;
  unsigned int ignored_cons = 0;
  unsigned int soft_cons = 0;

  vector<string> background_cons;

  for (auto &ci : fzn_model->constraints()) {
    string name = nameToPath.getName(con_id);
    if (n_cons <= con_id || isBackgroundConstraint(ci, name)) {
      hard_cons++;
      background_cons.emplace_back(name);
    } else if (isFilteredIn(ci, name)) {
      if (n_cons <= con_id) {
        error("Error: Path file does not match FlatZinc");
        exit(EXIT_FAILURE);
      }
      constraints[name] = &ci;
      if (mopts.subproblem_structure == STR_FLAT) {
        tree.children.push_back(MapNode(nameToPath.getPath(name), name));
      } else {
        string path;
        if (mopts.subproblem_structure !=
            STR_NORMAL) { // Not STR_FLAT and not STR_NORMAL
          path = utils::generalizeLabel(
              nameToPath.getPath(name),
              mopts.subproblem_structure == STR_IDX_MIX ||
                  mopts.subproblem_structure == STR_IDX,
              mopts.subproblem_structure == STR_GEN_MIX ||
                  mopts.subproblem_structure == STR_IDX_MIX);
        } else {
          path = nameToPath.getPath(name);
        }
        MapNode &last = tree.addPath(path);
        stringstream ss;
        vector<string> cons = utils::split(last.con_id, '#');
        cons.push_back(name);
        last.con_id = utils::join(cons, "#");
      }
      soft_cons++;
    } else { // Not background, not include by filter
      if (mopts.subproblem_filter_mode == FILTER_FOREGROUND) {
        hard_cons++;
        background_cons.emplace_back(name);
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
  } while (nbranches != cs.nbranches);

  if (mopts.subproblem_structure == STR_GEN)
    tree.mergeLeaves();

  if (mopts.subproblem_binarize == BIN_ALL) {
    tree.makeBinary([](const MapNode &n) { return n.children.size() > 2; });
  }

  cs = tree.getCounts();

  init_oracle_model(oraclepath, hard_cons + soft_cons, background_cons);

  if (mopts.output_stats) {
    std::chrono::duration<double> dur =
        std::chrono::system_clock::now() - start_build;
    std::stringstream ss;
    ss << "hard cons: " << hard_cons
       << " soft cons: " << soft_cons << " leaves: " << cs.nleaves
       << " branches: " << cs.nbranches << " Built tree in "
       << std::fixed << std::setprecision(5) << dur.count()
       << " seconds.";
    log(ss.str(), 0);
  }
}

bool FznSubProblem::provedSAT() { return last_sat; }

ConstraintSet FznSubProblem::getConstraintSet(const Selection &b) {
  set<string> leaf_names = b.getLeaves();
  ConstraintSet entries;
  for (const string &leaf_name : leaf_names) {
    string path = nameToPath.getPath(leaf_name);
    ConstraintInfo n;
    n.leaf_name = leaf_name;
    n.setAnnotatedNamesFrom(constraints[leaf_name]);

    vector<string> split_all = utils::split(path, MINOR_SEP);
    n.name = split_all.back();

    if (mopts.map_depth == DEPTH_INSTANCE) {
      vector<string> splitPath = utils::getPathHead(path, false, true);
      path = utils::join(splitPath, string(1, MAJOR_SEP));
    } else if (mopts.map_depth == DEPTH_CUSTOM) {
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

inline ShrunkSet FznSubProblem::getShrunk() {
  assert(!shrunk.con_ids.empty());
  return shrunk;
}

void FznSubProblem::setShrunk(const std::vector<int> &solver_mus, bool min) {
  shrunk.minimal = min;
  shrunk.con_ids.clear();
  for (int idx : solver_mus) {
    auto it = solverModelMapping.find(idx);
    if (it != solverModelMapping.end()) {
      shrunk.con_ids.insert(it->second);
    }
  }
}

bool FznSubProblem::check(const Selection &b) {
  MiniZinc::SolverInstance::Status s = MiniZinc::SolverInstance::ERROR;

  std::chrono::time_point<std::chrono::system_clock> beginCheck =
      std::chrono::system_clock::now();
  set<string> leaves = b.getLeaves();

  if (!oracle.children.empty()) {
    vector<string> leaves_vec(leaves.begin(), leaves.end());
    s = oracle.contains_subset(leaves_vec) ? MiniZinc::SolverInstance::UNSAT
                                           : MiniZinc::SolverInstance::SAT;
  }

  MiniZinc::SolverFactory *sf = nullptr;
  MiniZinc::SolverInstanceBase *si = nullptr;

  if (!mopts.oracle_only && s != MiniZinc::SolverInstance::UNSAT) {
    // Build solver
    MiniZinc::Timer startTime;
    MiniZinc::MznSolver solver(nullstream, mzn_log, startTime);

    // Build arguments for MznSolver
    vector<string> args;
    args.push_back(
        "minizinc"); // Make sure MznSolver knows to run in minizinc driver mode
    args.push_back("--solver");
    args.push_back(mopts.subproblem_solver);

    if (mopts.subproblem_native_shrink) {
      args.push_back("--diagnose");
    }

    vector<string> timeout_args;
    mopts.adjustSolverTimeout();
    timeout_args.push_back("--solver-time-limit");
    timeout_args.push_back(
        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                           mopts.subproblem_solver_time_limit)
                           .count()));

    vector<string> split_extra_args =
        utils::split(mopts.subproblem_solver_flags, ' ');
    args.insert(args.end(), split_extra_args.begin(), split_extra_args.end());

    // Mark all constraints as removed;
    for (auto &kv : constraints) {
      kv.second->remove();
    }
    // Activate the selected constraints
    for (const string &l : leaves) {
      constraints[l]->unremove();
    }

    shrunk.minimal = false;
    shrunk.con_ids.clear();

    // Mark all constraints as removed;
    for (auto &kv : constraints) {
      kv.second->remove();
    }

    int solver_con_id = 0;
    int con_id = 0;
    // Activate the selected constraints
    set<string> leaves = b.getLeaves();

    solverModelMapping.clear();

    for (unsigned int i = 0; i < fzn_model->size(); i++) {
      if (MiniZinc::ConstraintI *ci =
              (*fzn_model)[i]->dynamicCast<MiniZinc::ConstraintI>()) {
        if (!ci->removed()) {
          // Constraint is part of background
          solver_con_id++;
        } else {
          // Constraint is part of foreground but might not be added
          string con_id_s = std::to_string(con_id);
          if (leaves.find(con_id_s) != leaves.end()) {
            ci->unremove();
            solverModelMapping[solver_con_id] = con_id_s;
            solver_con_id++;
          }
        }
        con_id++;
      }
    }

    std::vector<string> args_vec(args);
    switch (solver.processOptions(args_vec)) {
    case 0:
      break;
    default:
      {
        stringstream ss;
        ss << "Error creating solver with args: "
           << utils::join(args, " ") << std::endl
           << mzn_log.str() << std::endl;
        error(ss.str());
      }
      exit(EXIT_FAILURE);
    }

    sf = solver.getSF();
    int i = 0;
    auto siOpt = solver.getSIOptions();
    sf->processOption(siOpt, i, timeout_args);

    si = sf->createSI(fzn_env, mzn_log, siOpt);
    si->setSolns2Out(&s2o);
    try {
      si->processFlatZinc();

      s = si->solve();
#ifdef MZN_SUPPORTS_SHRINK
      if (s == MiniZinc::SolverInstance::UNSAT) {
        auto statusMUS = si->getMUSStatus();

        if (statusMUS != MiniZinc::SolverInstance::MUS_NONE) {
          setShrunk(si->getMUS(),
                    statusMUS == MiniZinc::SolverInstance::MUS_MINIMAL);
        }
      }
#endif
      sf->destroySI(si);
    } catch (const MiniZinc::InternalError &err) {
      std::stringstream ess;
      ess << "Exception during sub-solving: " << err.msg()
          << ": " << err.what();
      error(ess.str());
      exit(EXIT_FAILURE);
    } catch (const MiniZinc::Error &err) {
      std::stringstream ess;
      ess << "Exception during sub-solving: " << err.msg()
          << ": " << err.what();
      error(ess.str());
      exit(EXIT_FAILURE);
    } catch (const std::runtime_error &err) {
      std::stringstream ess;
      ess << "Caught runtime error:" << std::endl << err.what();
      error(ess.str());
      exit(EXIT_FAILURE);
    } catch (...) {
      error("Caught unknown exception during sub-solving");
      string errfilename = "FINDMUS_failed_subproblem.fzn";
      saveFzn(b, errfilename);
      exit(EXIT_FAILURE);
    }

    std::string error_log = mzn_log.str();
    if (!error_log.empty()) {
      std::stringstream ess;
      ess << "stderr from MznSolver:\n" << error_log;
      error(ess.str());
      mzn_log.clear();
      exit(EXIT_FAILURE);
    }
  }

  static int timeout_count = 0;
  string res;

  bool is_sat = s != MiniZinc::SolverInstance::UNSAT;
  last_sat = false;
  if (s == MiniZinc::SolverInstance::SAT ||
      s == MiniZinc::SolverInstance::OPT) {
    last_sat = true;
    res = "S";
  } else if (s == MiniZinc::SolverInstance::UNSAT) {
    res = "U";
  } else if (s == MiniZinc::SolverInstance::ERROR) {
    res = "E";
    string errfilename = "FINDMUS_failed_subproblem.fzn";
    log("Solver reported error without message", 0);
    saveFzn(b, errfilename);
  } else {
    res = "?";
    if (mopts.subproblem_dump_timeouts) {
      stringstream timeout_filename;
      timeout_filename << "FINDMUS_timeout_" << timeout_count++ << ".fzn";
      log("Dumping timed out flatzinc", 0);
      saveFzn(b, timeout_filename.str());
    }
  }

  if (mopts.verbose_subsolve) {
    std::stringstream ss;

    ss << "Check: " << s << "(" << res << ":"
       << (is_sat ? "S" : "U") << ") ";
    if (mopts.verbose_subsolve > 1) {
      ss << "cons: " << b;
    } else {
      ss << "ncons: " << std::setw(8) << leaves.size();
    }
    std::chrono::duration<double> dur =
        std::chrono::system_clock::now() - beginCheck;
    ss << " took: " << std::fixed << std::setprecision(5) << dur.count()
       << " seconds mus: ("
       << !shrunk.con_ids.empty() << "," << shrunk.minimal << ")";
    log(ss.str());
  }

  return is_sat;
}

void FznSubProblem::saveFzn(const Selection &b, const string &filename) {
  {
    std::stringstream ss;
    ss << "Dumping fzn as: " << filename;
    log(ss.str(), 0);
  }

  set<string> leaves = b.getLeaves();
  // Mark all constraints as removed;
  for (auto &kv : constraints) {
    kv.second->remove();
  }
  // Activate the selected constraints
  for (const string &l : leaves) {
    constraints[l]->unremove();
  }

  std::ofstream f;
  f.open(filename);
  if (f.is_open()) {
    MiniZinc::Printer p(f, 0, true);
    for (auto &it : fzn_model->functions()) {
      if (!it.removed()) {
        p.print(&it);
      }
    }
    for (auto &it : fzn_model->vardecls()) {
      if (!it.removed()) {
        p.print(&it);
      }
    }
    for (auto &it : fzn_model->constraints()) {
      if (!it.removed()) {
        p.print(&it);
      }
    }
    p.print(fzn_model->solveItem());
    f.close();
  } else {
    std::stringstream ss;
    ss << "cannot open file" << filename << " for writing";
    log(ss.str(), 0);
  }
}

string FznSubProblem::getSolStr(const Selection &b) {
  ConstraintSet cs = getConstraintSet(b);
  return cs.getSummary(mopts.subproblem_output_format, mopts.map_depth);
}

void FznSubProblem::printSol(const Selection &b) {
  log(getSolStr(b), 0);
}

} // namespace HierMUS

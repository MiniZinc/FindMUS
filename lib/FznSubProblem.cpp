#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <limits>

#include <minizinc/file_utils.hh>
#include <minizinc/solver.hh>

#include "FznSubProblem.h"
#include "Types.h"
#include "string_utils.h"
#include "path_utils.h"

#ifdef _MSC_VER
#undef ERROR
#endif

namespace HierMUS {
  using std::string;
  using std::vector;
  using std::stringstream;
  using std::ifstream;
  using std::ofstream;
  using std::set;
  using std::unordered_map;

  void SetTrie::add_set(const vector<string>& set, int i) {
    if(i == set.size()) return;
    const string& val = set[i];
    bool term = i == set.size()-1;

    for(SetTrie& child : children) {
      if(child.value == val) {
        if(term) child.terminal = term;
        else child.add_set(set, i+1);
        return;
      }
    }
    children.emplace_back(val, term);
    if(!term) {
      children.back().add_set(set, i+1);
    }
  }

  bool SetTrie::contains_subset(const vector<string>& set, int i) {
    if(i == set.size()) return false;
    const string& val = set[i];
    bool found = false;

    for(SetTrie& child : children) {
      if(child.value == val) {
        found = child.terminal ? true : child.contains_subset(set, i+1);
        break;
      }
    }
    return found ? true : contains_subset(set, i+1);
  }

  void SetTrie::add_set(const vector<string>& set) {
    vector<string> sorted_set (set.begin(), set.end());
    std::sort(sorted_set.begin(), sorted_set.end());
    // std::cout << "Add set: {" << utils::join(sorted_set, ", ") << "}\n";
    add_set(sorted_set, 0);
  }

  bool SetTrie::contains_subset(const vector<string>& set) {
    vector<string> sorted_set (set.begin(), set.end());
    std::sort(sorted_set.begin(), sorted_set.end());
    bool b = contains_subset(sorted_set, 0);
    //std::cout << "contains_subset: {" << utils::join(sorted_set, ", ") << "} = " << b << "\n";
    return b;
  }

  NullSolns2Out::NullSolns2Out() : Solns2Out(nullstream, nullstream, "") {}
  NullSolns2Out::~NullSolns2Out() {}
  std::ostream& NullSolns2Out::getOutput() { return nullstream; }

  bool FznSubProblem::isBackgroundConstraint(const MiniZinc::ConstraintI& ci, const string& name) {
    if(nameToPath.getPath(name) == "NOPATH") return true;
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
      const string& path = nameToPath.getPath(name);
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

  void FznSubProblem::init_oracle_model(const string& oraclepath, int ncons, vector<string>& background_cons) {
    ifstream oracle_log(oraclepath);
    if (!oracle_log.is_open()) return;

    string line;
    while(std::getline(oracle_log, line)) {
      if(line.size() > 4 && line.substr(0,4) == "MUS:") {
        vector<string> cons = utils::split(line.substr(5, line.size()), ' ');
        oracle.add_set(cons);
      }
    }
  }

  void FznSubProblem::init_fzn_model(const string& fznpath) {
    MiniZinc::GCLock lock;
    vector<string> includes;

    includes.push_back(mopts.mzn_stdlib_dir + "/std/");

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
  }

  FznSubProblem::FznSubProblem(
      const string& fznpath, const string& pathpath,
      MUSEnumOptions& mo, const string& oraclepath = "")
      : SubProblem(mo), last_sat{false}, nameToPath{pathpath}, fzn_model{ nullptr }, oracle{ "root", false } {
    auto start_build = std::chrono::system_clock::now();

    init_fzn_model(fznpath);

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
    unsigned int soft_cons = 0;

    vector<string> background_cons;

    for(auto cit = fzn_model->begin_constraints(); cit != fzn_model->end_constraints(); ++cit) {
      MiniZinc::ConstraintI& ci = *cit;
      string name = nameToPath.getName(con_id);
      if(isBackgroundConstraint(ci, name)) {
        hard_cons++;
        background_cons.emplace_back(name);
      } else {
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

    init_oracle_model(oraclepath, hard_cons + soft_cons, background_cons);

    //if(mopts.verbose_subsolve) {
    std::chrono::duration<double> dur = std::chrono::system_clock::now() - start_build;
    std::cout << "FznSubProblem:\thard cons: " << hard_cons << "\tsoft cons: " << soft_cons << "\tleaves: " << cs.nleaves << "\tbranches: " << cs.nbranches << "\tBuilt tree in "
      << std::fixed << std::setprecision(5) << dur.count() << " seconds.\n";
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

  bool FznSubProblem::check(const Selection& b) {
    std::chrono::time_point<std::chrono::system_clock> beginCheck = std::chrono::system_clock::now();
    set<string> leaves = getLeaves(b);

    MiniZinc::SolverInstance::Status s = MiniZinc::SolverInstance::ERROR;

    if(!oracle.children.empty()) {
      vector<string> leaves_vec(leaves.begin(), leaves.end());
      s = oracle.contains_subset(leaves_vec) ? MiniZinc::SolverInstance::UNSAT : MiniZinc::SolverInstance::SAT;
    }

    if(!mopts.oracle_only && s != MiniZinc::SolverInstance::UNSAT) {
      // Build solver
      MiniZinc::MznSolver solver(nullstream, log);

      // Build arguments for MznSolver
      vector<string> args;
      args.push_back("minizinc"); // Make sure MznSolver knows to run in minizinc driver mode
      args.push_back("--solver");
      args.push_back(mopts.subproblem_solver);

      mopts.adjustSolverTimeout();
      args.push_back("--solver-time-limit");
      args.push_back(std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(mopts.subproblem_solver_time_limit).count()));
      vector<string> split_extra_args = utils::split(mopts.subproblem_solver_flags, ' ');
      args.insert(args.end(), split_extra_args.begin(), split_extra_args.end());
      // Mark all constraints as removed;
      for(auto& kv : constraints) { kv.second->remove(); }
      // Activate the selected constraints
      for(const string& l : leaves) { constraints[l]->unremove(); }

      std::vector<string> args_vec(args);
      switch (solver.processOptions(args_vec)) {
        case 0:
          break;
        default:
          std::cerr << "FznSubProblem:\tError creating solver with args:\t" << utils::join(args, " ") << std::endl;
          std::cerr << "\t" << log.str() << "\n";
          exit(EXIT_FAILURE);
      }

      MiniZinc::SolverFactory* sf = solver.getSF();
      MiniZinc::SolverInstanceBase* si = sf->createSI(fzn_env, log, solver.getSI_OPT());
      si->setSolns2Out(&s2o);
      si->processFlatZinc();

      try {
        s = si->solve();
      } catch (const MiniZinc::InternalError& err) {
        std::cerr << "FznSubproblem:\tException during sub-solving: "
          << err.msg() << ": " << err.what() << std::endl;
        exit(EXIT_FAILURE);
      }

      sf->destroySI(si);

      std::string error_log = log.str();
      if(!error_log.empty()) {
        std::cerr << "FznSubproblem:\tstderr from MznSolver:\n" << error_log << "\n";
        log.clear();
        exit(EXIT_FAILURE);
      }
    }

    string res;
    bool is_sat = s != MiniZinc::SolverInstance::UNSAT;
    last_sat = false;
    if (s == MiniZinc::SolverInstance::SAT || s == MiniZinc::SolverInstance::OPT) {
      last_sat = true;
      res = "S";
    } else if (s == MiniZinc::SolverInstance::UNSAT) {
      res = "U";
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
      std::chrono::duration<double> dur = std::chrono::system_clock::now() - beginCheck;
      std::cout << "\ttook: " << std::fixed << std::setprecision(5) << dur.count() << " seconds" << std::endl;
    }

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

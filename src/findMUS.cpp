#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <csignal>

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#undef ERROR
#endif

#include "HierMUSEnumer.h"
#include "OldMUSEnumer.h"

#include "NamePathMap.h"
#include "FznSubProblem.h"
#include "Options.h"

#ifdef BUILD_FINDMUS_EXAMPLES
#include "DemoSubProblems.h"
#include "FileSP.h"
#endif

#include <minizinc/solver.hh>
#include <minizinc/file_utils.hh>

using namespace HierMUS;
using std::string;

namespace FindMUSState {
static bool sigint = false;
static void regHandler();

#ifdef WIN32

static BOOL handlesig(DWORD sig) {
  if (sig != CTRL_C_EVENT) return false;
  sigint = true;
  regHandler();
  return true;
}

#else

static void handlesig(int sig) {
  sigint = true;
  regHandler();
}

#endif

static void regHandler() {
#ifdef WIN32
  if(!sigint)
    SetConsoleCtrlHandler( (PHANDLER_ROUTINE) handlesig, true);
#else
  if(!sigint)
    std::signal(SIGINT, handlesig);
#endif
}

static void run(DriverOptions& dro, MUSEnumOptions& mo, MusEnumerator& me) {
  regHandler();
  std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();
  Statistics& stats = me.getStatistics();
  if(dro.output_progress) { std::cout << "%%%mzn-progress 0.0" << std::endl; }
  int start_sat_calls = 0;
  while(!sigint && !mo.timedOut() && me.search()) {
    me.printMUS();
    std::cout << std::flush;
    stats.nmuses++;

    if(dro.maxmuses > 0) {
      if(dro.output_progress) { std::cout << "%%%mzn-progress " << (static_cast<float>(stats.nmuses) / dro.maxmuses * 100.0f) << std::endl; }
      if(stats.nmuses >= dro.maxmuses) break;
    }

    int sat_calls_d = stats.sat_calls - start_sat_calls;
    if(dro.frequent_stats) {
      std::chrono::duration<double> dur = std::chrono::system_clock::now() - start_time;
      std::cout << "Intermediate Result: Time: " << std::fixed << std::setprecision(5) << dur.count()
                << "\tnmuses: " << stats.nmuses << "\t" << stats << "\tdsat: " << sat_calls_d << std::endl;
    }
    start_sat_calls = stats.sat_calls;
  }

  if(sigint || mo.timedOut()) {
    me.printMUS();
  }

  if(dro.output_progress) { std::cout << "%%%mzn-progress 100.0" << std::endl; }
  if(dro.output_stats) {
    std::chrono::duration<double> dur = std::chrono::system_clock::now() - start_time;
    std::cout << "Total Time: " << std::fixed << std::setprecision(5) << dur.count()
      << "\tnmuses: " << stats.nmuses << "\t" << me.getStatistics() << std::endl;
  }
}
}

SubProblem* createProblem(DriverOptions& dro,
                          MUSEnumOptions& mo,
                          vector<MiniZinc::FileUtils::TmpFile>& temp_files) {
  SubProblem* problem = nullptr;
#ifdef BUILD_FINDMUS_EXAMPLES
  if(!dro.demo_name.empty()) {
    if(dro.demo_name == "hm5") problem = new HM5(mo);
    else if(dro.demo_name == "hm5_2") problem = new HM5_2(mo);
    else if(dro.demo_name == "file") problem = new FileSP(mo, dro.demo_path);
    else if(dro.demo_name == "glm") problem = new GLM(mo);
    else if(dro.demo_name == "fflat") problem = new FFLAT(mo);
    else if(dro.demo_name == "path") problem = new Path(mo);
    else if(dro.demo_name == "p1f") problem = new P1f(mo);
    else if(dro.demo_name == "rand") problem = new RandomProblem(mo, 
                                                                 dro.demo_rand_seed,
                                                                 dro.demo_rand_cons,
                                                                 dro.demo_rand_muses,
                                                                 dro.demo_rand_mus_size);
  }
#endif
  if(!problem) {
    if(dro.input_files.empty()) {
      std::cerr << "No input files provided" << std::endl;
      OptionsHelper::help_short(EXIT_FAILURE);
    }
    if(dro.fznpath.empty()) {
      MiniZinc::MznSolver solver(std::cerr, std::cerr);
      vector<string> args {
        "--stdlib-dir", mo.mzn_stdlib_dir,
        "-c",
        "--solver", mo.subproblem_solver
      };

      args.insert(args.end(), dro.input_files.begin(), dro.input_files.end());

      temp_files.reserve(2);
      temp_files.emplace_back(".fzn");
      dro.fznpath = temp_files.back().name();
      temp_files.emplace_back(".paths");
      dro.pathpath = temp_files.back().name();

      if(dro.compile_domains) args.push_back("-g");
      if(dro.compile_verbose) args.push_back("--verbose");
      if(dro.compile_stats) args.push_back("-s");

      args.push_back("-o");
      args.push_back(dro.fznpath);
      args.push_back("--output-paths-to-file");
      args.push_back(dro.pathpath);
      args.push_back("--no-output-ozn");

      std::vector<string> args_vec(args);
      try {
        if(solver.run(args_vec, "", "minizinc") == MiniZinc::SolverInstance::ERROR) {
          std::cerr << "FznSubProblem:\tError: Failed to compile model with args:\t" << utils::join(args, " ") << std::endl;
          exit(EXIT_FAILURE);
        }
      }  catch (const MiniZinc::LocationException& e) {
        std::cerr << std::endl;
        std::cerr << e.loc() << ":" << std::endl;
        std::cerr << e.what() << ": " << e.msg() << std::endl;
        exit(EXIT_FAILURE);
      } catch (const MiniZinc::Exception& e) {
        std::cerr << std::endl;
        std::string what = e.what();
        std::cerr << what << (what.empty() ? "" : ": ") << e.msg() << std::endl;
        exit(EXIT_FAILURE);
      } catch (const std::exception& e) {
        std::cerr << std::endl;
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
      } catch (...) {
        std::cerr << std::endl;
        std::cerr << "  UNKNOWN EXCEPTION." << std::endl;
        exit(EXIT_FAILURE);
      }
    }

    if(dro.fznpath.empty()) {
      std::cerr << "No flatzinc file provided" << std::endl;
      OptionsHelper::help_short(EXIT_FAILURE);
    } else {
      std::ifstream checkpath(dro.fznpath);
      if(!checkpath.is_open()) {
        std::cerr << "Flatzinc file "<< dro.fznpath << " cannot be opened." << std::endl;
        OptionsHelper::help_short(EXIT_FAILURE);
      }
    }
    if(!dro.oraclepath.empty()) {
      std::ifstream checkpath(dro.oraclepath);
      if(!checkpath.is_open()) {
        std::cerr << "Warning: Oracle file "<< dro.oraclepath << " not found." << std::endl;
        dro.pathpath = "";
      }
    }
    if(dro.pathpath.empty()) {
      string base = dro.fznpath.substr(0,dro.fznpath.length()-4);
      dro.pathpath = base + ".paths";
    }
    {
      std::ifstream checkpath(dro.pathpath);
      if(!checkpath.is_open()) {
        std::cerr << "Warning: Path file "<< dro.pathpath << " not found. Compile with --output-paths to produce .paths file. Using flat hierarchy." << std::endl;
        dro.pathpath = "";
      }
    }
    problem = new FznSubProblem(dro.fznpath, dro.pathpath, mo, dro.oraclepath);
  }
  return problem;
}

void writeDotFile(const DriverOptions& dro, SubProblem* problem){
  std::ofstream f(dro.dump_dot_path);
  if(!f.is_open()) {
    std::cout << "\nFailed to write to file" << std::endl;
    return;
  }
  DotWriter dw(f, problem->getTree());
  dw.print();
}


int main(int argc, char **argv) {
  Statistics s;
  DriverOptions dro;
  MUSEnumOptions mo(s);
  OptionsHelper::parse(dro, mo, argc, argv);

  if(dro.list_solvers || dro.list_solvers_json) {
    std::fstream nullstream;
    MiniZinc::MznSolver solver(nullstream, std::cout);
    vector<string> args (1);
    if(dro.list_solvers) args.push_back("--solvers");
    if(dro.list_solvers_json) args.push_back("--solvers-json");
    solver.run(args);
    exit(EXIT_SUCCESS);
  }

  vector<MiniZinc::FileUtils::TmpFile> temp_files;
  // Create SubProblem
  HierMUS::SubProblem* problem = createProblem(dro, mo, temp_files);

  if(!dro.dump_dot_path.empty()) {
    std::cout << "Writing diagram to: " << dro.dump_dot_path << " and exiting" << std::endl;
    writeDotFile(dro, problem);
    exit(EXIT_SUCCESS);
  }

  // Create MUS Enumerator
  MusEnumerator* me;
  if (dro.use_new_enumer) {
    me = new HierMUSEnumer(*problem, mo);
  } else {
    me = new OldMUSEnumer(*problem, mo);
  }

  // Sanity checks
  auto native_shrink = mo.subproblem_native_shrink;
  mo.subproblem_native_shrink = false; // Disable use of --diagnose flag for sanity check

  std::chrono::time_point<std::chrono::system_clock> check_unsat_starttime = std::chrono::system_clock::now();
  // Is the model unsat to begin with?
  if(problem->check(me->getRootSelector())) { // If unsat isn't proven, check returns SAT
    if(problem->provedSAT()) {
      std::cout << "Error: Model is Satisfiable" << std::endl;
      return EXIT_SUCCESS;
    } else {
      std::cout << "Error: Cannot prove UNSAT within solver timelimit. "
                << "Set a larger timeout with the '--solver-timelimit' argument." << std::endl;
    }
    return EXIT_FAILURE;
  }
  std::chrono::duration<double> check_unsat_time = std::chrono::system_clock::now() - check_unsat_starttime;

  // Is the background satisfiable:
  if(!problem->check(Selection())) {
    std::cout << "Background is not satisfiable, exiting. "
      << "Try using --soft-defines." << std::endl;
    return EXIT_FAILURE;
  }

  mo.subproblem_native_shrink = native_shrink; // Restore native setting
  if(mo.subproblem_adapt_time_limit) {
    auto scaled_unsat_time = 1.5 * check_unsat_time;
    mo.subproblem_solver_time_limit = scaled_unsat_time.count() < mo.subproblem_solver_time_limit.count()
                                      ? scaled_unsat_time
                                      : mo.subproblem_solver_time_limit;
    std::cout << "%% Adapting solver time limit to: " << std::chrono::duration_cast<std::chrono::seconds>(mo.subproblem_solver_time_limit).count() << std::endl;
  }

  // Print suggestion if using gen/hint
  if(mo.subproblem_structure == STR_GEN) {
    std::cout << "Note: Generalising model structure (stripping instance specific info). "
      << "Use \"--paramset mzn\" for more precise output" << std::endl;
  }

  FindMUSState::run(dro, mo, *me);

  delete me;

  return EXIT_SUCCESS;
}

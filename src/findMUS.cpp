#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <csignal>

#include "HierMUSEnumer.h"

#include "FznSubProblem.h"
#include "Options.h"

#ifdef BUILD_FINDMUS_EXAMPLES
#include "DemoSubProblems.h"
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
  double start_time = wallClockTime();
  int nmuses = 0;
  if(dro.output_progress) { std::cout << "%%%mzn-progress 0.0\n"; }
  while(!sigint && !mo.timedOut() && me.search()) {

    me.printMUS();
    std::cout << std::flush;
    nmuses++;

    if(dro.maxmuses > 0) {
      if(dro.output_progress) { std::cout << "%%%mzn-progress " << (static_cast<float>(nmuses) / dro.maxmuses * 100.0f) << std::endl; }
      if(nmuses >= dro.maxmuses) break;
    }
    if(dro.frequent_stats)
      std::cout << "Intermediate Result: Time: " << std::fixed << std::setprecision(5) << wallClockTime() - start_time
                << "\tnmuses: " << nmuses << "\t" << me.getStatistics() << "\n";
  }

  if(sigint || mo.timedOut()) {
    me.printMUS();
  }

  if(dro.output_progress) { std::cout << "%%%mzn-progress 100.0\n"; }
  std::cout << "Total Time: " << std::fixed << std::setprecision(5) << wallClockTime() - start_time
            << "\tnmuses: " << nmuses << "\t" << me.getStatistics() << "\n";
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
    else if(dro.demo_name == "glm") problem = new GLM(mo);
    else if(dro.demo_name == "fflat") problem = new FFLAT(mo);
    else if(dro.demo_name == "rand") problem = new RandomProblem(mo, 
                                                                 dro.demo_rand_seed,
                                                                 dro.demo_rand_cons,
                                                                 dro.demo_rand_muses,
                                                                 dro.demo_rand_mus_size);
  }
#endif
  if(!problem) {
    if(dro.input_files.empty()) {
      std::cerr << "No input files provided\n";
      OptionsHelper::help_short(EXIT_FAILURE);
    }
    if(dro.fznpath.empty()) {
      MiniZinc::MznSolver solver(std::cerr, std::cerr);
      vector<string> args { "-c", "--solver", mo.subproblem_solver };
      args.insert(args.end(), dro.input_files.begin(), dro.input_files.end());

      temp_files.reserve(2);
      temp_files.emplace_back(".fzn");
      dro.fznpath = temp_files.back().name();
      temp_files.emplace_back(".paths");
      dro.pathpath = temp_files.back().name();

      if(dro.compile_domains) args.push_back("-g");
      if(dro.compile_verbose) args.push_back("--verbose");

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
      std::cerr << "No flatzinc file provided\n";
      OptionsHelper::help_short(EXIT_FAILURE);
    } else {
      std::ifstream checkpath(dro.fznpath);
      if(!checkpath.is_open()) {
        std::cerr << "Flatzinc file "<< dro.fznpath << " cannot be opened.\n";
        OptionsHelper::help_short(EXIT_FAILURE);
      }
    }
    if(dro.pathpath.empty()) {
      string base = dro.fznpath.substr(0,dro.fznpath.length()-4);
      dro.pathpath = base + ".paths";
    }
    {
      std::ifstream checkpath(dro.pathpath);
      if(!checkpath.is_open()) {
        std::cerr << "Path file "<< dro.pathpath << " cannot be opened.\n";
        OptionsHelper::help_short(EXIT_FAILURE);
      }
    }
    problem = new FznSubProblem(dro.fznpath, dro.pathpath, mo);
  }
  return problem;
}

void writeDotFile(const DriverOptions& dro, SubProblem* problem){
  std::ofstream f(dro.dump_dot_path);
  if(!f.is_open()) {
    std::cout << "\nFailed to write to file\n";
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
    std::cout << "Writing diagram to: " << dro.dump_dot_path << " and exiting\n";
    writeDotFile(dro, problem);
    exit(EXIT_SUCCESS);
  }
  
  // Create MUS Enumerator
  MusEnumerator* me = new HierMUSEnumer(*problem, mo);

  // Is the model unsat to begin with?
  if(!dro.ignore_sat_model && problem->check(me->getRootSelector())) {
    std::cout << "Error: Cannot prove UNSAT within solver timelimit. Set a larger timeout with the\n"
              << "'--solver-timelimit' argument or use '--ignore-sat-model' flag to run MUS enumeration anyway.\n";
    return EXIT_FAILURE;
  }

  // Is the background satisfiable:
  if(!dro.ignore_unsatisfiable_background && !problem->check(Selection())) {
    std::cout << "Background is not satisfiable, exiting." << std::endl;
    return EXIT_FAILURE;
  }

  FindMUSState::run(dro, mo, *me);

  delete me;
}

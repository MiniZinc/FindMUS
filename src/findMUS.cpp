#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>

#include "HierMUSEnumer.h"

#include "FznSubProblem.h"
#include "Options.h"

#ifdef BUILD_FINDMUS_EXAMPLES
#include "DemoSubProblems.h"
#endif

using namespace HierMUS;
using std::string;

SubProblem* createProblem(DriverOptions& dro,
                             MUSEnumOptions& mo) {
  SubProblem* problem = nullptr;
#ifdef BUILD_FINDMUS_EXAMPLES
  if(!dro.demo_name.empty()) {
    if(dro.demo_name == "hm5") problem = new HM5(mo);
    else if(dro.demo_name == "hm5_2") problem = new HM5_2(mo);
    else if(dro.demo_name == "fflat") problem = new FFLAT(mo);
  }
#endif
  if(!problem) {
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
  DriverOptions dro;
  MUSEnumOptions mo;
  OptionsHelper::parse(dro, mo, argc, argv);

  HierMUS::SubProblem* problem = createProblem(dro, mo);

  if(!dro.dump_dot_path.empty()) {
    std::cout << "Writing diagram to: " << dro.dump_dot_path << " and exiting\n";
    writeDotFile(dro, problem);
    exit(EXIT_SUCCESS);
  }

  // Are you using remus in Hierarchical Mode
  if(mo.subproblem_structure != STR_FLAT && mo.map_enumeration_alg == ALG_REMUS) {
    std::cout << "Warning: Using ReMUS as HierMUS's sub-MUS-enumerator is not currently supported."
              << "Use --structure flat for accurate MUSes.\n";
  }
  HierMUS::HierMUSEnumer me(*problem, mo);
  // Is the model unsat to begin with?
  if(!dro.ignore_sat_model && problem->check(me.getRootSelector())) {
    std::cout << "Error: Cannot prove UNSAT within solver timelimit. Set a larger timeout with the\n"
              << "'--solver-timelimit' argument or use '--ignore-sat-model' flag to run MUS enumeration anyway.\n";
    return EXIT_FAILURE;
  }
  // Is the background satisfiable:
  if(!dro.ignore_unsatisfiable_background && !problem->check(Selection())) {
    std::cout << "Background is not satisfiable, exiting." << std::endl;
    return EXIT_FAILURE;
  }

  Statistics& stats = me.getStatistics();
  double start_time = wallClockTime();
  int nmuses = 0;
  if(dro.output_progress) { std::cout << "%%%mzn-progress 0.0\n"; }
  while(!mo.timedOut(stats) && me.search()) {
    if(dro.colour) std::cout << "\033[1;31m";
    me.printMUS();
    std::cout << std::flush;
    if(dro.colour) std::cout << "\033[0m";
    nmuses++;
    if(dro.maxmuses > 0) {
      if(dro.output_progress) { std::cout << "%%%mzn-progress " << (static_cast<float>(nmuses) / dro.maxmuses * 100.0) << std::endl; }
      if(nmuses >= dro.maxmuses) break;
    }
    if(dro.frequent_stats)
      std::cout << "Intermediate Result: Time: " << std::fixed << std::setprecision(5) << wallClockTime() - start_time 
                << "\tnmuses: " << nmuses << "\t" << me.getStatistics() << "\n";
  }
  if(dro.output_progress) { std::cout << "%%%mzn-progress 100.0\n"; }
  std::cout << "Total Time: " << std::fixed << std::setprecision(5) << wallClockTime() - start_time 
            << "\tnmuses: " << nmuses << "\t" << me.getStatistics() << "\n";

}

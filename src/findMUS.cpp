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

void printHelp() {
  std::cout << "findMUS: Explain an unsatisfiable model\n"
    << "  usage: findMUS [flatzinc file] [paths file]\n"
    << "                 [--marco] [--verbose[-X]] [--demo <demo>]\n"
    << "                 [--nmuses <n>] [--output-minizinc]\n"
    << "                 [--binarize {none,all,leaves}]\n"
    << "                 [--structure {flat,normal,gen,mix}]\n"
    << "                 [--soft-defines] \n"
    << "                 [--model-depth] [--depth <n>]\n"
    << "                 [--frequent-stats] [--timeout <s>]\n"
    << "                 [--output-html] [--output-brief]\n"
    << "                 [--solver <s>] [--solver-flags <f>]\n"
    << "                 [--solver-timelimit <ms>]\n"
    << "                \n"
    << "  flatzinc file\n"
    << "    Unsatisfiable flatzinc model\n"
    << "  paths file\n"
    << "    Symbol table for unsatisfiable flatzinc model\n"
    << "\n"
    << " Driver Options:\n"
    << "  --nmuses <n>\n"
    << "    Number of MUSes to find\n";
#ifdef BUILD_FINDMUS_EXAMPLES
  std::cout << "  --demo <demo>\n"
    << "    Use demo model and tree. <string> must be one of:\n"
    << "      hm5, hm5_2\n";
#endif
  std::cout << "  --timeout <s>\n"
    << "    Stop after <s> seconds. Default: 1800 seconds\n"
    << "  --frequent-stats\n"
    << "    Output high-level stats after each MUS (for logging)\n"
    << "  --output-html\n"
    << "    Output html for use with MiniZincIDE\n"
    << "  --output-brief\n"
    << "    Don't output traces (useful for debugging)\n"
    << "\n"
    << " Enumeration Options:\n"
    << "  --marco\n"
    << "    Use MARCO algorithm as sub-enumerator\n"
    << "  --remus\n"
    << "    Use ReMUS algorithm as sub-enumerator\n"
    << "  --qx\n"
    << "    Use quickXplain with MARCO or ReMUS (not used by HierMUS)\n"
    << "  --depth mzn,fzn,<n>\tdefault: mzn\n"
    << "    Enumerate MUSes at the level of:\n"
    << "      mzn: the user's model\n"
    << "      fzn: the program level constraints (decomposition)\n"
    << "      <n>: integer, a custom depth\n"
    << "\n"
    << " Subproblem options\n"
    << "  --solver <s>\n"
    << "    Use solver <s> for SAT checking. Default: \"fzn-gecode\"\n"
    << "  --solver-flags <f>\n"
    << "    Pass flags <f> to solver for SAT checking. Default: \"-time 1000\"\n"
    << "  --solver-timelimit <ms>\n"
    << "    Hard time limit for solver in milliseconds. Default: 1100\n"
    << "  --ignore-unsat-background\n"
    << "    Skip unsatisfiable background check\n"
    << "  --structure flat,gen,normal,mix\n"
    << "    Alters initial structure: (Default: normal)\n"
    << "      flat:   Remove all structure\n"
    << "      gen:    Remove instance specific structure\n"
    << "      normal: No change\n"
    << "      mix:    Apply 'gen' before 'normal'\n"
    << "  --binarize normal,leaves,all\n"
    << "    Add additional structure: (Default: normal)\n"
    << "      normal: no change\n"
    << "      leaves: introduce structure at the leaves\n"
    << "      all:    introduce structure throughout tree\n"
    << "  --soft-defines\n"
    << "    Consider functional constraints as part of MUSes\n"
    << "  --named-only\n"
    << "    Only consider constraints annotated with string annotations\n"
    << "  --named-filter <names>\n"
    << "    Only consider constraints annotated with string annotations that\n"
    << "    match comma separated <names>\n"
    << "  --named-filter-exclude <names>\n"
    << "    Ignore constraints annotated with string annotations that\n"
    << "    match comma separated <names>\n"
    << "  --path-filter <paths>\n"
    << "    Only consider constraints with paths containing strings from\n"
    << "    comma separated <paths>\n"
    << "  --path-filter-exclude <paths>\n"
    << "    Ignore constraints with paths containing strings from\n"
    << "    comma separated <paths>\n"
    << "  --filter-sep <sep>\n"
    << "    Separator used for named and path filters\n"
    << "  --dump-dot <dot>\n"
    << "    Write tree to file <dot>\n"
    << "\n"
    << " Verbosity Options:\n"
    << "  --verbose-enum\n"
    << "    Output verbose information from Enumerator\n"
    << "  --verbose-enum-iteration-stats\n"
    << "    Output stats at the end of each iteration\n"
    << "  --verbose-map\n"
    << "    Output verbose information from SubsetMap\n"
    << "  --verbose-subsolve\n"
    << "    Output verbose information from subsolver\n"
    << "  --verbose-subsolve-extra\n"
    << "    Output more verbose information from subsolver\n"
    << "  --verbose\n"
    << "    Output more information during enumeration\n"
    << "\n";
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  string fznpath;
  string pathpath;
  int maxmuses = 0;
  bool frequent_stats = false;
  string dump_dot_path;
  char filter_sep = ',';
  bool ignore_unsafisfiable_background = false;
#ifdef BUILD_FINDMUS_EXAMPLES
  string demo_name;
#endif

  bool isPipe = !isatty(fileno(stdout));

  MUSEnumOptions mo;

  HierMUS::SubProblem* problem = NULL;
  for(int i=1; i<argc; i++) {
    if(strcmp(argv[i], "--help") == 0) {
      printHelp();
    } else if(strcmp(argv[i], "--marco") == 0) {
      mo.map_enumeration_alg = ALG_MARCO;
    } else if(strcmp(argv[i], "--remus") == 0) {
      mo.map_enumeration_alg = ALG_REMUS;
    } else if(strcmp(argv[i], "--qx") == 0) {
      mo.map_shrink_alg = SH_QX;
    } else if(strcmp(argv[i], "--structure") == 0) {
      std::string type = argv[++i];
      if(type == "flat")        mo.subproblem_structure = STR_FLAT;
      else if(type == "normal") mo.subproblem_structure = STR_NORMAL;
      else if(type == "gen")    mo.subproblem_structure = STR_GEN;
      else if(type == "mix")    mo.subproblem_structure = STR_GEN_MIX;
      else printHelp();
    } else if(strcmp(argv[i], "--binarize") == 0) {
      std::string type = argv[++i];
      if(type == "none")        mo.subproblem_binarize = BIN_NONE;
      else if(type == "leaves") mo.subproblem_binarize = BIN_LEAVES;
      else if(type == "all")    mo.subproblem_binarize = BIN_EVERYWHERE;
      else printHelp();
    } else if(strcmp(argv[i], "--ignore-unsat-background") == 0) {
      ignore_unsafisfiable_background = true;
    } else if(strcmp(argv[i], "--nmuses") == 0) {
      maxmuses = atoi(argv[++i]);
    } else if(strcmp(argv[i], "--timeout") == 0) {
      mo.timeout = atof(argv[++i]);
    } else if(strcmp(argv[i], "--output-minizinc") == 0) {
      mo.output_minizinc = true;
    } else if(strcmp(argv[i], "--solver") == 0) {
      mo.subproblem_solver = argv[++i];
    } else if(strcmp(argv[i], "--solver-flags") == 0) {
      mo.subproblem_solver_flags = argv[++i];
    } else if(strcmp(argv[i], "--solver-timelimit") == 0) {
      mo.subproblem_solver_time_limit = atoi(argv[++i]);
    } else if(strcmp(argv[i], "--depth") == 0) {
      std::string a = argv[++i];
      if(isdigit(a[0])) {
        mo.map_depth = DEPTH_CUSTOM;
        mo.map_depth_max = atoi(a.c_str());
      } else {
        mo.map_depth_max = 0;
        if(a == "mzn") mo.map_depth = DEPTH_INSTANCE;
        else if(a == "fzn") mo.map_depth = DEPTH_PROGRAM;
        else printHelp();
      }
    } else if(strcmp(argv[i], "--soft-defines") == 0) {
      mo.subproblem_hard_functional_constraints = false;
    } else if(strcmp(argv[i], "--named-only") == 0) {
      mo.subproblem_named_only = true;
    } else if(strcmp(argv[i], "--filter-sep") == 0) {
      filter_sep = argv[++i][0];
    } else if(strcmp(argv[i], "--named-filter") == 0 ||
              strcmp(argv[i], "--named-filter-include") == 0) {
      std::string csfilter = argv[++i];
      vector<string> filters = utils::split(csfilter, filter_sep);
      mo.subproblem_name_filters.insert(filters.begin(), filters.end());
    } else if(strcmp(argv[i], "--named-filter-exclude") == 0) {
      std::string csfilter = argv[++i];
      vector<string> filters = utils::split(csfilter, filter_sep);
      mo.subproblem_name_filters_excludes.insert(filters.begin(), filters.end());
    } else if(strcmp(argv[i], "--path-filter") == 0 ||
              strcmp(argv[i], "--path-filter-include") == 0) {
      std::string csfilter = argv[++i];
      vector<string> filters = utils::split(csfilter, filter_sep);
      mo.subproblem_path_filters.insert(filters.begin(), filters.end());
    } else if(strcmp(argv[i], "--path-filter-exclude") == 0) {
      std::string csfilter = argv[++i];
      vector<string> filters = utils::split(csfilter, filter_sep);
      mo.subproblem_path_filters_excludes.insert(filters.begin(), filters.end());
    } else if(strcmp(argv[i], "--dump-dot") == 0) {
      dump_dot_path = argv[++i];
    } else if(strcmp(argv[i], "--verbose") == 0) {
      mo.verbose_final_stats = true;
      mo.verbose_enum_iteration_stats = true;
      mo.verbose_enum = true;
      mo.verbose_map = true;
      mo.verbose_subsolve = true;
    } else if(strcmp(argv[i], "--verbose-enum") == 0) {
      mo.verbose_enum = true;
    } else if(strcmp(argv[i], "--verbose-enum-iteration-stats") == 0) {
      mo.verbose_enum_iteration_stats = true;
    } else if(strcmp(argv[i], "--verbose-map") == 0) {
      mo.verbose_map = true;
    } else if(strcmp(argv[i], "--verbose-subsolve") == 0) {
      mo.verbose_subsolve = true;
    } else if(strcmp(argv[i], "--verbose-subsolve-extra") == 0) {
      mo.verbose_subsolve = true;
      mo.verbose_subsolve_extra = true;
    } else if(strcmp(argv[i], "--frequent-stats") == 0) {
      frequent_stats = true;
    } else if(strcmp(argv[i], "--output-html") == 0) {
      mo.subproblem_output_format = OUT_HTML;
    } else if(strcmp(argv[i], "--output-brief") == 0) {
      mo.subproblem_output_format = OUT_DEBUG;
#ifdef BUILD_FINDMUS_EXAMPLES
    } else if(strcmp(argv[i], "--demo") == 0) {
      demo_name = argv[++i];
#endif
    } else {
      if(argv[i][0] == '-') {
        std::cerr << "Unknown argument: " << argv[i] << "\n";
        printHelp();
      } else if(fznpath.empty()) {
        fznpath = argv[i];
      } else if(pathpath.empty()) {
        pathpath = argv[i];
      } else {
        std::cerr << "Unknown argument: " << argv[i] << "\n";
        printHelp();
      }
    }
  }

#ifdef BUILD_FINDMUS_EXAMPLES
  if(!demo_name.empty()) {
    if(demo_name == "hm5") problem = new HM5(mo);
    else if(demo_name == "hm5_2") problem = new HM5_2(mo);
    else if(demo_name == "fflat") problem = new FFLAT(mo);
  }
#endif

  if(!problem) {
    if(fznpath.empty()) {
      std::cerr << "No flatzinc file provided\n";
      printHelp();
    }
    if(pathpath.empty()) {
      string base = fznpath.substr(0,fznpath.length()-4);
      pathpath = base + ".paths";
    }
    problem = new FznSubProblem(fznpath, pathpath, mo);
  }

  if(!dump_dot_path.empty()) {
    std::cout << "Writing diagram to: " << dump_dot_path << " and exiting\n";
    std::ofstream f(dump_dot_path);
    if(!f.is_open()) {
      std::cout << "\nFailed to write to file\n";
      exit(EXIT_SUCCESS);
    }
    DotWriter dw(f, problem->getTree());
    dw.print();
    f.close();
    exit(EXIT_SUCCESS);
  }

  // Sanity checks:
  if(!mo.subproblem_name_filters_excludes.empty()) {
    if(!mo.subproblem_name_filters.empty()) {
      for(const std::string& exclude : mo.subproblem_name_filters_excludes) {
        if(mo.subproblem_name_filters.find(exclude) != mo.subproblem_name_filters.end()) {
          mo.subproblem_name_filters.erase(exclude);
          mo.subproblem_name_filters_excludes.erase(exclude);
        }
      }
    }
  }

  // Are you using remus in Hierarchical Mode
  if(mo.subproblem_structure != STR_FLAT && mo.map_enumeration_alg == ALG_REMUS) {
    std::cout << "Warning: Using ReMUS as HierMUS's sub-MUS-enumerator is not currently supported."
              << "Use --structure flat for accurate MUSes.\n";
  }
  // Is the background satisfiable:
  if(!ignore_unsafisfiable_background && !problem->check(Selection())) {
    std::cout << "Background is not satisfiable, exiting." << std::endl;
    return EXIT_FAILURE;
  }
  HierMUS::HierMUSEnumer me(*problem, mo);
  Statistics& stats = me.getStatistics();

  double start_time = wallClockTime();
  int nmuses = 0;
  bool html_output = mo.subproblem_output_format == OUT_HTML;
  if(html_output) {
    std::cout << "%%%mzn-html-start\n";
  }
  while(!mo.timedOut(stats) && me.search()) {
    if(html_output) {
      std::cout << "Mus: " << nmuses << " :";
      me.printMUS();
      std::cout << "<br/>\n";
    } else {
      if(!isPipe) std::cout << "\033[1;31m";
      std::cout << "MUS: ";
      me.printMUS();
      if(!isPipe) std::cout << "\033[0m";
    }
    nmuses++;
    if(maxmuses > 0 && nmuses >= maxmuses) break;
    if(frequent_stats)
      std::cout << "Intermediate Result: Time: " << std::fixed << std::setprecision(5) << wallClockTime() - start_time 
                << "\tnmuses: " << nmuses << "\t" << me.getStatistics() << "\n";
  }
  if(html_output) {
    std::cout << "%%%mzn-html-end\n";
  }
  std::cout << "Total Time: " << std::fixed << std::setprecision(5) << wallClockTime() - start_time 
            << "\tnmuses: " << nmuses << "\t" << me.getStatistics() << "\n";
}

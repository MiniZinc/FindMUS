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

void help_short(int exit_code = EXIT_SUCCESS) {
  std::cout << "findMUS: Explain an unsatisfiable model\n"
    << "  usage: findMUS <flatzinc file> [paths file]\n"
    << "                 [--ignore-unsat-background]\n"
    << "                 [--structure {normal, flat, gen, mix}]\n"
    << "                 [--binarize {none, leaves, all}]\n"
    << "                 [--depth {mzn, fzn, i}\n"
    << "                 [--verbose-{enum,map,subsolve} <v>]\n"
    << "                 [--verbose]\n"
    << "\n";
  if(exit_code == EXIT_FAILURE)
    exit(EXIT_FAILURE);
}

void help_long() {
  help_short();
  std::cout
    << "  flatzinc file\n"
    << "    Unsatisfiable flatzinc model\n"
    << "  paths file\n"
    << "    Symbol table for unsatisfiable flatzinc model\n"
    << "\n"
    << " Driver Options:\n"
    << "  --nmuses <n>\n"
    << "    Number of MUSes to find\n"
    << "  --stdlib-dir <path>\n"
    << "    Set path to MiniZinc standard library\n"
    << "   --ignore-unsat-background\n"
    << "     Skip unsatisfiable background check\n";
#ifdef BUILD_FINDMUS_EXAMPLES
  std::cout << "  --demo <demo>\n"
    << "    Use demo model and tree. <string> must be one of:\n"
    << "      hm5, hm5_2\n";
#endif
  std::cout
    << "  --paths <path>\n"
    << "    Explicit path to paths file\n"
    << "  --timeout <s>\n"
    << "    Stop after <s> seconds. Default: 1800 seconds\n"
    << "  --frequent-stats\n"
    << "    Output high-level stats after each MUS (for logging)\n"
    << "  --output-{html, brief}\n"
    << "    Output modes, html for use with MiniZincIDE, brief for testing\n"
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
    << " Subproblem: Solving options\n"
    << "  --solver <s>\n"
    << "    Use solver <s> for SAT checking. Default: \"fzn-gecode\"\n"
    << "  --solver-flags <f>\n"
    << "    Pass flags <f> to solver for SAT checking. Default: \"-time 1000\"\n"
    << "  --solver-timelimit <ms>\n"
    << "    Hard time limit for solver in milliseconds. Default: 1100\n"
    << "  Subproblem filtering options:\n"
    << "   --soft-defines\n"
    << "     Consider functional constraints as part of MUSes\n"
    << "   --named-only\n"
    << "     Only consider constraints annotated with string annotations\n"
    << "   --filter-named <names>    --filter-named-exclude <names>\n"
    << "     Include/exclude constraints with names that match <sep> separated <names>\n"
    << "   --filter-path <paths>    --filter-path-exclude <paths>\n"
    << "     Include/exclude based on <paths>\n"
    << "   --filter-sep <sep>\n"
    << "     Separator used for named and path filters\n"
    << "  Subproblem structure options:\n"
    << "   --structure flat,gen,normal,mix\n"
    << "     Alters initial structure: (Default: normal)\n"
    << "       flat:   Remove all structure\n"
    << "       gen:    Remove instance specific structure\n"
    << "       normal: No change\n"
    << "       mix:    Apply 'gen' before 'normal'\n"
    << "   --binarize normal,leaves,all\n"
    << "     Add additional structure: (Default: normal)\n"
    << "       normal: no change\n"
    << "       leaves: introduce structure at the leaves\n"
    << "       all:    introduce structure throughout tree\n"
    << "\n"
    << " Verbosity Options:\n"
    << "  --verbose-{map,enum,subsolve} <n>:\n"
    << "    Set verbosity level for different components\n"
    << "  --verbose\n"
    << "    Set verbosity level of all components to 1\n"
    << "\n"
    << " Misc Options:\n"
    << "  --dump-dot <dot>\n"
    << "    Write tree in GraphViz format to file <dot>\n"
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
      help_long();
    } else if(strcmp(argv[i], "--marco") == 0) {
      mo.map_enumeration_alg = ALG_MARCO;
    } else if(strcmp(argv[i], "--remus") == 0) {
      mo.map_enumeration_alg = ALG_REMUS;
    } else if(strcmp(argv[i], "--qx") == 0) {
      mo.map_shrink_alg = SH_QX;
    } else if(strcmp(argv[i], "--stdlib-dir") == 0) {
      mo.mzn_stdlib_dir = argv[++i];
    } else if(strcmp(argv[i], "--structure") == 0) {
      std::string type = argv[++i];
      if(type == "flat")        mo.subproblem_structure = STR_FLAT;
      else if(type == "normal") mo.subproblem_structure = STR_NORMAL;
      else if(type == "gen")    mo.subproblem_structure = STR_GEN;
      else if(type == "mix")    mo.subproblem_structure = STR_GEN_MIX;
      else {
        std::cout << "Incorrect structure setting. Available options are {flat, <normal>, gen, mix}\n";
        help_short(EXIT_FAILURE);
      }
    } else if(strcmp(argv[i], "--binarize") == 0) {
      std::string type = argv[++i];
      if(type == "none")        mo.subproblem_binarize = BIN_NONE;
      else if(type == "leaves") mo.subproblem_binarize = BIN_LEAVES;
      else if(type == "all")    mo.subproblem_binarize = BIN_EVERYWHERE;
      else {
        std::cout << "Incorrect binarize option. Available options are {<none>, leaves, all}\n";
        help_short(EXIT_FAILURE);
      }
    } else if(strcmp(argv[i], "--ignore-unsat-background") == 0) {
      ignore_unsafisfiable_background = true;
    } else if(strcmp(argv[i], "--nmuses") == 0) {
      maxmuses = atoi(argv[++i]);
      if(maxmuses == 1) {
        mo.map_enum_focus_mode = true;
      }
    } else if(strcmp(argv[i], "--timeout") == 0) {
      mo.timeout = atof(argv[++i]);
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
        else {
          std::cout << "Incorrect depth setting: Valid options are {<fzn>, mzn} or a positive integer\n";
          help_short(EXIT_FAILURE);
        }
      }
    } else if(strcmp(argv[i], "--soft-defines") == 0) {
      mo.subproblem_hard_functional_constraints = false;
    } else if(strcmp(argv[i], "--named-only") == 0) {
      mo.subproblem_named_only = true;
    } else if(strcmp(argv[i], "--filter-sep") == 0) {
        filter_sep = argv[++i][0];
    } else if(!string(argv[i]).compare(0, 8, "--filter")) {
      std::string arg = argv[i];
      std::string csfilter = argv[++i];
      vector<string> filters = utils::split(csfilter, filter_sep);
      if(arg == "--filter-named-exclude") {
        mo.subproblem_name_filters_excludes.insert(filters.begin(), filters.end());
      } else if(arg == "--filter-named") {
        mo.subproblem_name_filters.insert(filters.begin(), filters.end());
      } else if(arg == "--filter-path-exclude") {
        mo.subproblem_path_filters_excludes.insert(filters.begin(), filters.end());
      } else if(arg == "--filter-path") {
        mo.subproblem_path_filters.insert(filters.begin(), filters.end());
      }
    } else if(strcmp(argv[i], "--dump-dot") == 0) {
      dump_dot_path = argv[++i];
    } else if(strcmp(argv[i], "--verbose") == 0) {
      mo.verbose_final_stats = true;
      mo.verbose_enum = 1;
      mo.verbose_map = 1;
      mo.verbose_subsolve = 1;
    } else if(strcmp(argv[i], "--verbose-enum") == 0) {
      mo.verbose_enum = atoi(argv[++i]);
    } else if(strcmp(argv[i], "--verbose-map") == 0) {
      mo.verbose_map = atoi(argv[++i]);
    } else if(strcmp(argv[i], "--verbose-subsolve") == 0) {
      mo.verbose_subsolve = atoi(argv[++i]);
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
    } else if(strcmp(argv[i], "--paths") == 0) {
      pathpath = argv[++i];
    } else {
      if(argv[i][0] == '-') {
        std::cerr << "Unknown argument: " << argv[i] << "\n";
        help_short(EXIT_FAILURE);
      } else if(fznpath.empty()) {
        fznpath = argv[i];
      } else if(pathpath.empty()) {
        pathpath = argv[i];
      } else {
        std::cerr << "Unknown argument: " << argv[i] << "\n";
        help_short(EXIT_FAILURE);
      }
    }
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

  if(mo.mzn_stdlib_dir.empty()) {
    mo.mzn_stdlib_dir = MiniZinc::FileUtils::share_directory();
    if(mo.mzn_stdlib_dir == "") {
      std::cerr << "Please set MZN_STDLIB_DIR or pass --stdlib-dir argument.\n";
      help_short(EXIT_FAILURE);
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
      help_short(EXIT_FAILURE);
    } else {
      std::ifstream checkpath(fznpath);
      if(!checkpath.is_open()) {
        std::cerr << "Flatzinc file "<< fznpath << " cannot be opened.\n";
        help_short(EXIT_FAILURE);
      }
    }
    if(pathpath.empty()) {
      string base = fznpath.substr(0,fznpath.length()-4);
      pathpath = base + ".paths";
    }
    {
      std::ifstream checkpath(pathpath);
      if(!checkpath.is_open()) {
        std::cerr << "Path file "<< pathpath << " cannot be opened.\n";
        help_short(EXIT_FAILURE);
      }
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
      std::cout << "<br/>" << std::endl;
    } else {
      if(!isPipe) std::cout << "\033[1;31m";
      std::cout << "MUS: ";
      me.printMUS();
      std::cout << std::endl;
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

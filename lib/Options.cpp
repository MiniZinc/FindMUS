#include <minizinc/file_utils.hh>

#include "Options.h"
#include <vector>

namespace HierMUS {

namespace OptionsHelper {
void help_short(int exit_code) {
  std::cout << "findMUS: Explain an unsatisfiable model\n"
            << "  version: 0.5.0\n"
            << "  usage: findMUS <flatzinc file> [paths file]\n"
            << "                 [-a] [-n <n>]\n"
            << "                 [--paramset {hint, mzn, fzn}]\n"
            << "                 [--structure {normal, flat, gen, mix, idx, idxmix}]\n"
            << "                 [--no-binarize]\n"
            << "                 [--adapt-timelimit]\n"
            << "                 [--depth {mzn, fzn, i}\n"
            << "                 [--verbose-{enum,map,subsolve} <v>]\n"
            << "                 [--verbose-compile]\n"
            << "                 [--verbose]\n"
            << "\n";
  if(exit_code == EXIT_FAILURE)
    exit(EXIT_FAILURE);
}

void help_long(void) {
  help_short();
  std::cout
      << "  flatzinc file\n"
      << "    Unsatisfiable flatzinc model\n"
      << "  paths file\n"
      << "    Symbol table for unsatisfiable flatzinc model\n"
      << "\n"
      << " Driver Options:\n"
      << "  --paramset hint,mzn,fzn. Default: hint\n"
      << "    Use preset parameter sets:\n"
      << "      hint: --structure gen --shrink-alg map_lin --depth mzn\n"
      << "      mzn: --shrink-alg map_qx --depth mzn\n"
      << "      fzn: --shrink-alg map_qx --depth fzn\n"
      << "  -n <n>   --nmuses <n>\n"
      << "    Number of MUSes to find\n"
      << "  -a\n"
      << "    Find all MUSes\n"
      << "  --stdlib-dir <path>\n"
      << "    Set path to MiniZinc standard library\n";
#ifdef BUILD_FINDMUS_EXAMPLES
  std::cout << "  --demo <demo>\n"
            << "    Use demo model and tree. <string> must be one of:\n"
            << "      hm5, hm5_2, fflat, glm, rand\n"
            << "  --demo-rand-seed <n>\n"
            << "    For use with 'rand' demo, random seed\n"
            << "  --demo-rand-cons <n>\n"
            << "    For use with 'rand' demo, number of 'constraints'\n"
            << "  --demo-rand-muses <n>\n"
            << "    For use with 'rand' demo, number of MUSes\n"
            << "  --demo-rand-mus-size <n>\n"
            << "    For use with 'rand' demo, max constraints in a MUS\n";
#endif
  std::cout
      << "  --paths <path>\n"
      << "    Explicit path to paths file\n"
      << "  -t <ms>\n"
      << "    Stop after <ms> seconds. Default: -1 (no timelimit)\n"
      << "  --no-leftover \n"
      << "    Do not print leftover candidate on timeout\n"
      << "  --frequent-stats\n"
      << "    Output high-level stats after each MUS (for logging)\n"
      << "  --no-progress\n"
      << "    Do not output progress directive for IDE showing progress towards\n"
      << "    finding target number of MUSes.\n"
      << "  --output-{html, json, brief}\n"
      << "    Output modes, html for use with MiniZincIDE, brief for testing, json\n"
      << "    for easier to parse output.\n"
      << "  --use-old-enumer\n"
      << "    Use old approach (for testing only).\n"
      << "\n"
      << " Compiler Options:\n"
      << "  --domains  (-g)\n"
      << "    Record domain changes during compilation\n"
      << "  --verbose-compile\n"
      << "    Send --verbose to mzn2fzn\n"
      << "  -D<data>\n"
      << "    Include the given data assignment in the model.\n"
      << "\n"
      << " Enumeration Options:\n"
      << "  --shrink-alg lin,map_lin,qx,map_qx\tdefault: lin\n"
      << "    Shrink algorithm to use:\n"
      << "      lin:     linear shrink\n"
      << "      map_lin: linear shrink recording intermediate results in map\n"
      << "      qx:      use QuickXplain for shrink\n"
      << "      map_qx:  use advanced map driven QuickXplain\n"
      << "  --depth mzn,fzn,<n>\tdefault: 1\n"
      << "    Enumerate MUSes at the level of:\n"
      << "      mzn: the user's model\n"
      << "      fzn: the program level constraints (decomposition)\n"
      << "      <n>: integer, a custom depth\n"
      << "  --restarts\n"
      << "    Enable restart to flat mode if large run of sat sets encountered\n"
      << "  --seed <n>\n"
      << "    Set random seed\n"
      << "  --remus\n"
      << "    Use ReMUS instead of MARCO. A hierarchical mode has not been\n"
      << "    implemented. This should be used with '--no-binarize --structure flat'\n"
      << "\n"
      << " Subproblem: Solving options\n"
      << "  --solver <s>, --subsolver <s>\n"
      << "    Use solver <s> for SAT checking. Default: \"fzn-gecode\"\n"
      << "  --solver-flags <f>\n"
      << "    Pass flags <f> to solver for SAT checking. Default: \"-time 1000\"\n"
      << "  --adapt-timelimit\n"
      << "    Base solver timelimit on solve time for sanity checks.\n"
      << "    Maximum timelimit is provided by --solver-timelimit option\n"
      << "  --solver-timelimit <ms>, --subsolver-timelimit\n"
      << "    Hard time limit for solver in milliseconds. Default: 1000\n"
      << "  --native-shrink\n"
      << "    Request unsat sets from the subsolver (--diagnose must be supported by solver)\n"
      << "  Subproblem filtering options:\n"
      << "   --soft-defines\n"
      << "     Consider functional constraints as part of MUSes\n"
      << "   --hard-domains\n"
      << "     Consider domain constraints as part of the background\n"
      << "   --filter-mode fg,ex\n"
      << "     Change filter mode (Default: fg)\n"
      << "       fg: Foreground, excluded items go to the background\n"
      << "       ex: Exclusive, excluded items are omitted entirely\n"
      << "   --filter-named <names>   --filter-named-exclude <names>\n"
      << "     Include/exclude constraints with names that match <sep> separated <names>\n"
      << "     A special wildcard string of \"*\" matches all named constraints\n"
      << "   --filter-path <paths>    --filter-path-exclude <paths>\n"
      << "     Include/exclude based on <paths>\n"
      << "   --filter-sep <sep>\n"
      << "     Separator used for named and path filters\n"
      << "  Subproblem structure options:\n"
      << "   --structure flat,gen,normal,mix,idx,idxmix\n"
      << "     Alters initial structure: (Default: normal)\n"
      << "       flat:   Remove all structure\n"
      << "       gen:    Remove instance specific structure\n"
      << "       normal: No change\n"
      << "       mix:    Apply 'gen' before 'normal'\n"
      << "       idx:    Remove all location inforation\n"
      << "       idxmix: Apply 'idx' before 'normal'\n"
      << "   --no-binarize\n"
      << "     Do not add binary structure\n"
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

void parse(DriverOptions& dro, MUSEnumOptions& mo, int argc, char**argv) {
  std::vector<std::string> args;
  for(int i=1; i<argc; i++) args.push_back(argv[i]);

  parse(dro, mo, args);
}

vector<string> expandParamSet(const vector<string>& in_args) {
  vector<string> args;

  for(int i=0; i<in_args.size(); i++) {
    if(in_args[i] == "--paramset") {
      const string& s = in_args[++i];
      if(s == "hint") {
        args.push_back("--structure"); args.push_back("gen");
        args.push_back("--shrink-alg"); args.push_back("map_lin");
        args.push_back("--depth"); args.push_back("mzn");
      } else if(s == "mzn") {
        args.push_back("--structure"); args.push_back("normal");
        args.push_back("--shrink-alg"); args.push_back("map_qx");
        args.push_back("--depth"); args.push_back("mzn");
      } else if(s == "fzn") { 
        args.push_back("--structure"); args.push_back("normal");
        args.push_back("--shrink-alg"); args.push_back("map_qx");
        args.push_back("--depth"); args.push_back("fzn");
      } else {
        std::cout << "Unknown paramset. Available options are {hint, mzn, fzn}\n";
        help_short(EXIT_FAILURE);
      }

    } else {
      args.push_back(in_args[i]);
    }
  }

  return args;
}

void parse(DriverOptions& dro, MUSEnumOptions& mo, const std::vector<std::string>& in_args) {
  vector<string> args = expandParamSet(in_args);

  for(int i=0; i<args.size(); i++) {
    if(args[i] == "--help") {
      help_long();
    } else if(args[i] == "--print-args") {
      std::cout << "Args: " << utils::join(args, " ") << std::endl;
    } else if(args[i] == "--paramset") {
      std::cout << "Paramset argument should have been expanded. This should not happen.\n";
      help_short(EXIT_FAILURE);
    } else if(args[i][0] == '-' && args[i][1] == 'D') {
      dro.input_files.push_back(args[i]);
    } else if(args[i] == "--shrink-frontier") {
      mo.map_shrink_frontier = true;
    } else if(args[i] == "--native-shrink") {
      mo.subproblem_native_shrink = true;
    } else if(args[i] == "--shrink-alg") {
      std::string alg = args[++i];
      if(alg == "lin")          mo.map_shrink_alg = SH_LIN;
      else if(alg == "map_lin") mo.map_shrink_alg = SH_MAP_LIN;
      else if(alg == "qx")      mo.map_shrink_alg = SH_QX;
      else if(alg == "map_qx")  mo.map_shrink_alg = SH_MAP_QX;
      else {
        std::cout << "Incorrect shrink option. Available options are {<lin>, map_lin, qx, map_qx}\n";
        help_short(EXIT_FAILURE);
      }
    } else if(args[i] ==  "--stdlib-dir") {
      mo.mzn_stdlib_dir = args[++i];
    } else if(args[i] ==  "--no-progress") {
      dro.output_progress = false;
    } else if(args[i] ==  "--structure") {
      std::string type = args[++i];
      if(type == "flat")        mo.subproblem_structure = STR_FLAT;
      else if(type == "normal") mo.subproblem_structure = STR_NORMAL;
      else if(type == "gen")    mo.subproblem_structure = STR_GEN;
      else if(type == "mix")    mo.subproblem_structure = STR_GEN_MIX;
      else if(type == "idx")    mo.subproblem_structure = STR_IDX;
      else if(type == "idxmix") mo.subproblem_structure = STR_IDX_MIX;
      else {
        std::cout << "Incorrect structure setting. Available options are {flat, <normal>, gen, mix}\n";
        help_short(EXIT_FAILURE);
      }
    } else if(args[i] ==  "--no-binarize") {
      mo.subproblem_binarize = BIN_NONE;
    } else if(args[i] ==  "--restarts") {
      mo.restarts_enabled = true;
    } else if(args[i] == "--nmuses" || args[i] ==  "-n") {
      dro.maxmuses = std::stoi(args[++i]);
      if(dro.maxmuses != 1) { // Disable focus mode
        mo.map_enum_focus_mode = false;
      }
    } else if(args[i] ==  "-a") {
      dro.maxmuses = 0; // Find all MUSes
      mo.map_enum_focus_mode = false; // Don't use focus mode
    } else if(args[i] ==  "-t") {
      mo.timelimit = std::stof(args[++i]);
    } else if(args[i] == "--no-leftover") {
      mo.print_leftover = false;
    } else if(args[i] ==  "--seed") {
      mo.setRandSeed(std::stoi(args[++i]));
    } else if(args[i] ==  "--remus") {
      mo.map_enumeration_alg = ALG_REMUS;
    } else if(args[i] ==  "--solvers") {
      dro.list_solvers = true;
    } else if(args[i] ==  "--solvers-json") {
      dro.list_solvers_json = true;
    } else if(args[i] == "--subsolver" || args[i] ==  "--solver") {
      mo.subproblem_solver = args[++i];
    } else if(args[i] ==  "--solver-flags") {
      mo.subproblem_solver_flags = args[++i];
    } else if(args[i] == "--subsolver-timelimit" || args[i] ==  "--solver-timelimit") {
      mo.subproblem_solver_time_limit = std::stoi(args[++i]);
    } else if(args[i] == "--adapt-timelimit") {
      mo.subproblem_adapt_time_limit = true;
    } else if(args[i] ==  "--depth") {
      std::string a = args[++i];
      if(isdigit(a[0])) {
        mo.map_depth = DEPTH_CUSTOM;
        mo.map_depth_max = static_cast<unsigned int>(std::stoi(a.c_str()));
      } else {
        mo.map_depth_max = 0;
        if(a == "mzn") mo.map_depth = DEPTH_INSTANCE;
        else if(a == "fzn") mo.map_depth = DEPTH_PROGRAM;
        else {
          std::cout << "Incorrect depth setting: Valid options are {<fzn>, mzn} or a positive integer\n";
          help_short(EXIT_FAILURE);
        }
      }
    } else if(args[i] ==  "--soft-defines") {
      mo.subproblem_hard_functional_constraints = false;
    } else if(args[i] ==  "--hard-domains") {
      mo.subproblem_hard_domain_constraints = true;
    } else if(args[i] ==  "--named-only") {
      mo.subproblem_name_filters.clear();
      mo.subproblem_name_filters.emplace_back("*");
    } else if(args[i] ==  "--filter-sep") {
      dro.filter_sep = args[++i][0];
    } else if(args[i] == "--filter-mode") {
      std::string mode = args[++i];
      if(mode == "fg")      mo.subproblem_filter_mode = FILTER_FOREGROUND;
      else if(mode == "ex") mo.subproblem_filter_mode = FILTER_EXCLUSIVE;
      else {
        std::cout << "Invalid filter mode. Available options are {fg, ex}\n";
        help_short(EXIT_FAILURE);
      }
    } else if(!string(args[i]).compare(0, 8, "--filter")) {
      std::string arg = args[i];
      std::string csfilter = args[++i];
      vector<string> filters = utils::split(csfilter, dro.filter_sep);
      if(arg == "--filter-named-exclude") {
        mo.subproblem_name_filters_excludes.insert(
            mo.subproblem_name_filters_excludes.end(),
            filters.begin(), filters.end());
      } else if(arg == "--filter-named") {
        mo.subproblem_name_filters.insert(
            mo.subproblem_name_filters.end(),
            filters.begin(), filters.end());
      } else if(arg == "--filter-path-exclude") {
        mo.subproblem_path_filters_excludes.insert(
            mo.subproblem_path_filters_excludes.end(),
            filters.begin(), filters.end());
      } else if(arg == "--filter-path") {
        mo.subproblem_path_filters.insert(
            mo.subproblem_path_filters.end(),
            filters.begin(), filters.end());
      }
    } else if(args[i] ==  "--dump-dot") {
      dro.dump_dot_path = args[++i];
    } else if(args[i] == "--verbose" || args[i] ==  "-v") {
      mo.verbose_final_stats = true;
      mo.verbose_enum = 1;
      mo.verbose_map = 1;
      mo.verbose_subsolve = 1;
      dro.compile_verbose = true;
    } else if(args[i] ==  "--verbose-compile") {
      dro.compile_verbose = true;
    } else if(args[i] == "-s") {
      dro.compile_stats = true;
    } else if(args[i] == "--domains" || args[i] ==  "-g") {
      dro.compile_domains = true;
    } else if(args[i] ==  "--verbose-enum") {
      mo.verbose_enum = static_cast<unsigned int>(std::stoi(args[++i]));
    } else if(args[i] ==  "--verbose-map") {
      mo.verbose_map = static_cast<unsigned int>(std::stoi(args[++i]));
    } else if(args[i] ==  "--verbose-subsolve") {
      mo.verbose_subsolve = static_cast<unsigned int>(std::stoi(args[++i]));
    } else if(args[i] ==  "--frequent-stats") {
      dro.frequent_stats = true;
    } else if(args[i] ==  "--output-json") {
      mo.subproblem_output_format = OUT_JSON;
    } else if(args[i] ==  "--output-html") {
      mo.subproblem_output_format = OUT_HTML;
    } else if(args[i] ==  "--output-brief") {
      mo.subproblem_output_format = OUT_DEBUG;
    } else if(args[i] ==  "--use-old-enumer") {
      dro.use_new_enumer = false;
#ifdef BUILD_FINDMUS_EXAMPLES
    } else if(args[i] ==  "--demo") {
      dro.demo_name = args[++i];
      if(dro.demo_name == "file") {
        dro.demo_path = args[++i];
      }
    } else if(args[i] ==  "--demo-rand-seed") {
      dro.demo_rand_seed = std::stoi(args[++i]);
    } else if(args[i] ==  "--demo-rand-cons") {
      dro.demo_rand_cons = std::stoi(args[++i]);
    } else if(args[i] ==  "--demo-rand-muses") {
      dro.demo_rand_muses = std::stoi(args[++i]);
    } else if(args[i] ==  "--demo-rand-mus-size") {
      dro.demo_rand_mus_size = std::stoi(args[++i]);
#endif
    } else {
      if(args[i] ==  "-") {
        std::cerr << "No support for reading from stdin (-)\n";
        help_short(EXIT_FAILURE);
      }
      if(args[i][0] == '-') {
        std::cout << "Unknown argument: " << args[i] << "\n";
        help_short(EXIT_FAILURE);
      } else {
        dro.input_files.push_back(args[i]);
      }
    }
  }

  // Update fznpath and pathpath if available
  for(const string& p : dro.input_files) {
    if(p.size() > 3) {
      if(p.substr(p.size()-3, 3) == "fzn") {
        if(!dro.fznpath.empty()) {
          std::cerr << "No support for multiple FlatZinc (.fzn) input files: " << p << "\n";
          help_short(EXIT_FAILURE);
        }
        dro.fznpath = p;
      }
      if(p.substr(p.size()-3, 3) == "ths") {
        if(!dro.pathpath.empty()) {
          std::cerr << "No support for multiple paths (.paths) input files: " << p << "\n";
          help_short(EXIT_FAILURE);
        }
        dro.pathpath = p;
      }
    } else {
      std::cerr << "Unknown file: " << p << std::endl;
      help_short(EXIT_FAILURE);
    }
  }

  // Sanity checks:
  if(!mo.subproblem_name_filters_excludes.empty()) {
    if(!mo.subproblem_name_filters.empty()) {
      for(const std::string& exclude : mo.subproblem_name_filters_excludes) {
        if(find(mo.subproblem_name_filters.begin(),
                mo.subproblem_name_filters.end(),
                exclude) != mo.subproblem_name_filters.end()) {
          mo.subproblem_name_filters.erase(
              std::remove(mo.subproblem_name_filters.begin(),
                mo.subproblem_name_filters.end(), exclude),
              mo.subproblem_name_filters.end());
          mo.subproblem_name_filters_excludes.erase(
              std::remove(mo.subproblem_name_filters_excludes.begin(),
                mo.subproblem_name_filters_excludes.end(), exclude),
              mo.subproblem_name_filters_excludes.end());
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
}

}

}

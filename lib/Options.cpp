#include <minizinc/file_utils.hh>

#include "Options.h"

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
      << "      hint: --structure gen --shrink-alg map_lin --binarize all --depth mzn\n"
      << "      mzn: --shrink-alg map_qx --binarize all --depth mzn\n"
      << "      fzn: --shrink-alg map_qx --binarize all --depth fzn\n"
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
      << "  --frequent-stats\n"
      << "    Output high-level stats after each MUS (for logging)\n"
      << "  --no-progress\n"
      << "    Do not output progress directive for IDE showing progress towards\n"
      << "    finding target number of MUSes.\n"
      << "  --output-{html, json, brief}\n"
      << "    Output modes, html for use with MiniZincIDE, brief for testing, json\n"
      << "    for easier to parse output.\n"
      << "\n"
      << " Compiler Options:\n"
      << "  --domains  (-g)\n"
      << "    Record domain changes during compilation\n"
      << "  --verbose-compile\n"
      << "    Send --verbose to mzn2fzn\n"
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
      << "  --no-restarts\n"
      << "    Do not restart to flat mode if large run of sat sets encountered\n"
      << "  --seed <n>\n"
      << "    Set random seed\n"
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
      << "   --hard-domains\n"
      << "     Consider domain constraints as part of the background\n"
      << "   --named-only\n"
      << "     Only consider constraints annotated with string annotations\n"
      << "   --filter-named <names>    --filter-named-exclude <names>\n"
      << "     Include/exclude constraints with names that match <sep> separated <names>\n"
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


void setParamSet(ParamSet ps, MUSEnumOptions& mo) {
  mo.sense = ps;
  if(ps == PSET_HINT) {
    mo.subproblem_structure = STR_GEN;
    mo.map_shrink_alg = SH_MAP_LIN;
    mo.subproblem_binarize = BIN_ALL;
    mo.map_depth = DEPTH_INSTANCE;
  } else if(ps == PSET_MZN) {
    mo.subproblem_structure = STR_NORMAL;
    mo.map_shrink_alg = SH_MAP_QX;
    mo.subproblem_binarize = BIN_ALL;
    mo.map_depth = DEPTH_INSTANCE;
  } else if(ps == PSET_FZN) {
    mo.subproblem_structure = STR_NORMAL;
    mo.map_shrink_alg = SH_MAP_QX;
    mo.subproblem_binarize = BIN_ALL;
    mo.map_depth = DEPTH_PROGRAM;
  }
}

void parse(DriverOptions& dro, MUSEnumOptions& mo, int argc, char**argv) {
  setParamSet(PSET_HINT, mo);
  for(int i=1; i<argc; i++) {
    if(strcmp(argv[i], "--help") == 0) {
      help_long();
    } else if(strcmp(argv[i], "--paramset") == 0) {
      std::string s = argv[++i];
      if(s == "hint") {
        setParamSet(PSET_HINT, mo);
      } else if(s == "mzn") {
        setParamSet(PSET_MZN, mo);
      } else if(s == "fzn") { 
        setParamSet(PSET_FZN, mo);
      } else {
        std::cout << "Unknown paramset. Available options are {hint, mzn, fzn}\n";
        help_short(EXIT_FAILURE);
      }
    } else if(strcmp(argv[i], "--shrink-alg") == 0) {
      std::string alg = argv[++i];
      if(alg == "lin")          mo.map_shrink_alg = SH_LIN;
      else if(alg == "map_lin") mo.map_shrink_alg = SH_MAP_LIN;
      else if(alg == "qx")      mo.map_shrink_alg = SH_QX;
      else if(alg == "map_qx")  mo.map_shrink_alg = SH_MAP_QX;
      else {
        std::cout << "Incorrect shrink option. Available options are {<lin>, map_lin, qx, map_qx}\n";
        help_short(EXIT_FAILURE);
      }
    } else if(strcmp(argv[i], "--stdlib-dir") == 0) {
      mo.mzn_stdlib_dir = argv[++i];
    } else if(strcmp(argv[i], "--no-progress") == 0) {
      dro.output_progress = false;
    } else if(strcmp(argv[i], "--structure") == 0) {
      std::string type = argv[++i];
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
    } else if(strcmp(argv[i], "--no-binarize") == 0) {
      mo.subproblem_binarize = BIN_NONE;
    } else if(strcmp(argv[i], "--no-restarts") == 0) {
      mo.restarts_enabled = false;
    } else if(strcmp(argv[i], "--nmuses") == 0 || strcmp(argv[i], "-n") == 0) {
      dro.maxmuses = atoi(argv[++i]);
      if(dro.maxmuses != 1) { // Disable focus mode
        mo.map_enum_focus_mode = false;
      }
    } else if(strcmp(argv[i], "-a") == 0) {
      dro.maxmuses = 0; // Find all MUSes
      mo.map_enum_focus_mode = false; // Don't use focus mode
    } else if(strcmp(argv[i], "-t") == 0) {
      mo.timelimit = atof(argv[++i]);
    } else if(strcmp(argv[i], "--seed") == 0) {
      mo.setRandSeed(atoi(argv[++i]));
    } else if(strcmp(argv[i], "--solvers") == 0) {
      dro.list_solvers = true;
    } else if(strcmp(argv[i], "--solvers-json") == 0) {
      dro.list_solvers_json = true;
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
        mo.map_depth_max = static_cast<unsigned int>(atoi(a.c_str()));
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
    } else if(strcmp(argv[i], "--hard-domains") == 0) {
      mo.subproblem_hard_domain_constraints = true;
    } else if(strcmp(argv[i], "--named-only") == 0) {
      mo.subproblem_named_only = true;
    } else if(strcmp(argv[i], "--filter-sep") == 0) {
      dro.filter_sep = argv[++i][0];
    } else if(!string(argv[i]).compare(0, 8, "--filter")) {
      std::string arg = argv[i];
      std::string csfilter = argv[++i];
      vector<string> filters = utils::split(csfilter, dro.filter_sep);
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
      dro.dump_dot_path = argv[++i];
    } else if(strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
      mo.verbose_final_stats = true;
      mo.verbose_enum = 1;
      mo.verbose_map = 1;
      mo.verbose_subsolve = 1;
      dro.compile_verbose = true;
    } else if(strcmp(argv[i], "--verbose-compile") == 0) {
      dro.compile_verbose = true;
    } else if(strcmp(argv[i], "--domains") == 0 || strcmp(argv[i], "-g") == 0) {
      dro.compile_domains = true;
    } else if(strcmp(argv[i], "--verbose-enum") == 0) {
      mo.verbose_enum = static_cast<unsigned int>(atoi(argv[++i]));
    } else if(strcmp(argv[i], "--verbose-map") == 0) {
      mo.verbose_map = static_cast<unsigned int>(atoi(argv[++i]));
    } else if(strcmp(argv[i], "--verbose-subsolve") == 0) {
      mo.verbose_subsolve = static_cast<unsigned int>(atoi(argv[++i]));
    } else if(strcmp(argv[i], "--frequent-stats") == 0) {
      dro.frequent_stats = true;
    } else if(strcmp(argv[i], "--output-json") == 0) {
      mo.subproblem_output_format = OUT_JSON;
    } else if(strcmp(argv[i], "--output-html") == 0) {
      mo.subproblem_output_format = OUT_HTML;
    } else if(strcmp(argv[i], "--output-brief") == 0) {
      mo.subproblem_output_format = OUT_DEBUG;
#ifdef BUILD_FINDMUS_EXAMPLES
    } else if(strcmp(argv[i], "--demo") == 0) {
      dro.demo_name = argv[++i];
    } else if(strcmp(argv[i], "--demo-rand-seed") == 0) {
      dro.demo_rand_seed = atoi(argv[++i]);
    } else if(strcmp(argv[i], "--demo-rand-cons") == 0) {
      dro.demo_rand_cons = atoi(argv[++i]);
    } else if(strcmp(argv[i], "--demo-rand-muses") == 0) {
      dro.demo_rand_muses = atoi(argv[++i]);
    } else if(strcmp(argv[i], "--demo-rand-mus-size") == 0) {
      dro.demo_rand_mus_size = atoi(argv[++i]);
#endif
    } else {
      if(argv[i][0] == '-') {
        std::cerr << "No support for reading from stdin: " << argv[i] << "\n";
        help_short(EXIT_FAILURE);
      }
      dro.input_files.push_back(argv[i]);
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
}

}

}

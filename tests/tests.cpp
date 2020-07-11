#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <chrono>

#include "HierMUSEnumer.h"
#include "FznSubProblem.h"
#include "Options.h"
#include "FileSP.h"

#include "../include/string_utils.h"

namespace HierMUS {
  using std::string;
  using std::vector;

  bool run_test(size_t id, const vector<string>& args, string path) {
    Statistics stats;
    DriverOptions dro;
    MUSEnumOptions mo(stats);

    auto n_args = OptionsHelper::expandParamSet(args);
    OptionsHelper::parse(dro, mo, n_args);

    FileSP filesp(mo, path);

    HierMUS::SubProblem* problem = static_cast<HierMUS::SubProblem*>(&filesp);
    HierMUSEnumer me(*problem, mo);

    string result = "PASS";

    while (!mo.timedOut() && me.search()) {
      const Selection& s = me.getCurrentMUS();
      if (filesp.isMUS(s)) {
        stats.nmuses++;
      } else {
        result = "BADMUS";
        break;
      }
    }
    if (mo.timedOut()) result = "TIMEOUT";
    if (stats.nmuses != filesp.muses.size()) result = "MISSED";

    const Statistics& mstats = me.getStatistics();
    std::cout << id << ": " << utils::join({
        path, (result == "PASS" ? "PASS" : ("FAIL("+result+")")),
        std::to_string(mstats.map_calls),
        std::to_string(mstats.sat_calls),
        std::to_string(stats.nmuses),
        utils::join(n_args, " "),
        }, "\t") << std::endl;

    return result == "PASS";
  }

  int run_tests(vector<size_t> ids) {
    vector<vector<string> > test_args = {
      {"-t", "1000", "--paramset", "fzn", "--no-binarize"},
      {"-t", "1000", "--paramset", "fzn", "--shrink-alg", "lin"},
      {"-t", "1000", "--paramset", "fzn", "--shrink-alg", "qx"},
      {"-t", "1000", "--paramset", "fzn", "--shrink-alg", "qx2"},
      {"-t", "1000", "--paramset", "fzn", "--shrink-alg", "map_lin"},
      {"-t", "1000", "--paramset", "fzn", "--no-binarize", "--structure", "flat", "--remus", "--shrink-alg", "map_lin"},
    };
    vector<string> spec_files = {
      "../examples/specs/dep.mus",
      "../examples/specs/indep.mus",
      "../examples/specs/hm5.mus",
      "../examples/specs/latin.mus",
      "../examples/specs/p1f.mus"
    };

    struct tpair {
      int args;
      int spec;
    };

    vector<struct tpair> tests;

    for(int sf=0; sf<spec_files.size(); sf++) {
      for(int ta=0; ta<test_args.size(); ta++) {
        tests.push_back( { ta, sf } );
      }
    }

    bool group_by_args = false;
    if(group_by_args) {
      std::sort(tests.begin(), tests.end(), [](const tpair& a, const tpair& b) {return a.args < b.args;});
    }

    if(ids.empty()) {
      for(int i=0; i<tests.size(); i++) {
        ids.push_back(i);
      }
    }

    bool all_pass = true;
    vector<string> fails;

    for(size_t id : ids) {
      auto& t = tests.at(id);
      bool result = run_test(id, test_args[t.args], spec_files[t.spec]);
      if(!result) fails.push_back(std::to_string(id));
      all_pass = all_pass && result;
    }

    std::cout << "Passed: (" << ids.size() - fails.size() << "/" << ids.size() << ")\t";
    std::cout << "Failed: (" << fails.size() << "/" << ids.size() << "):\t";
    if(!fails.empty()) {
      std::cout << utils::join(fails, " ");
    }
    std::cout << std::endl;

    return all_pass;
  }
}

int main(int argc, char** argv) {
  std::vector<size_t> ids;
  size_t i = 1;
  while(i < argc) {
    std::string s = std::string(argv[i]);
    auto tests = utils::split(s, ',');
    for(auto& t : tests) {
      ids.push_back(std::stoi(t));
    }
    i++;
  }

  return HierMUS::run_tests(ids);
}


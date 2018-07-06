#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <set>
#include <algorithm>

#include "DemoSubProblems.h"
#include "string_utils.h"

namespace HierMUS {
  using std::vector;
  using std::string;
  using std::set;

  // HM5
  HM5::HM5(MUSEnumOptions& mo) : SubProblem(mo) {
    MapNode b1 {"b1", "b1"}; MapNode b2 {"b2", "b2"}; MapNode b3 {"b3", "b3"};
    MapNode b4 {"b4", "b4"}; MapNode b5 {"b5", "b5"}; MapNode b6 {"b6", "b6"};
    leaf_names = {"b1", "b2", "b3", "b4", "b5", "b6"};

    if(mo.subproblem_structure == STR_FLAT) {
      tree.children = {b1,b2,b3,b4,b5,b6};
    } else {
      MapNode c12 {"12", {b1, b2}}; MapNode c34 {"34", {b3, b4}}; MapNode c56 {"56", {b5, b6}};
      MapNode c1234 {"1234", {c12, c34}};
      tree.children = {c1234, c56};
    }
  }

  void HM5::printSol(const Selection& b) {
    set<string> leaves = getLeaves(b);
    for(const string& leaf : leaves) {
      std::cout << leaf << ", ";
    }
    std::cout << "\n";
  }

  bool HM5::check(const Selection& s) {
    vector<bool> b(6, false);

    set<string> leaves = getLeaves(s);
    for(const string& leaf : leaves) {
      if(leaf == "b1") b[0] = true;
      if(leaf == "b2") b[1] = true;
      if(leaf == "b3") b[2] = true;
      if(leaf == "b4") b[3] = true;
      if(leaf == "b5") b[4] = true;
      if(leaf == "b6") b[5] = true;
    }

    bool sol = !((b[0] && b[1]) || (b[0] && b[2]) ||  (b[4] && b[5]) || (b[1] && b[2] && b[5]));
    //sol = !(b[0] &&b[1] &&b[2] &&b[3] &&b[4] &&b[5]);

    if(mopts.verbose_subsolve)
      std::cout << "HM5::check(" << s << ") :" << sol<<"\n";
    return sol;
  }

  // FFLAT
  FFLAT::FFLAT(MUSEnumOptions& mo) : SubProblem(mo) {
    int n = 16;
    //int n = 8;
    std::vector<MapNode> nodes;

    for(int i=15; i<15+n; i++) {
    //for(int i=7; i<7+n; i++) {
      std::stringstream ss;
      ss << i;
      std::string name = ss.str();
      nodes.push_back(MapNode{name, name});
    }

    tree.children = nodes;
    if(mopts.subproblem_binarize == BIN_LEAVES) {
      tree.makeBinary([](const MapNode& n){ 
        return n.children.size() > 2;
      });
    }
  }

  void FFLAT::printSol(const Selection& b) {
    set<string> leaves = getLeaves(b);
    for(const string& leaf : leaves) {
      std::cout << leaf << ", ";
    }
    std::cout << "\n";
  }

  bool FFLAT::check(const Selection& s) {
    set<string> leaves = getLeaves(s);
    vector<set<string> > unsat_sets;
    unsat_sets.push_back({ "15", "16" });
    unsat_sets.push_back({ "16", "17" });
    unsat_sets.push_back({ "17", "18" });
    //unsat_sets.push_back({ "7", "8" });
    //unsat_sets.push_back({ "8", "9" });
    //unsat_sets.push_back({ "9", "10" });

    bool sol = true;
    for(set<string>& us : unsat_sets) {
      if(std::includes(leaves.begin(), leaves.end(),
                       us.begin(), us.end()))
        sol = false;
    }

    if(mopts.verbose_subsolve)
      std::cout << "FFLAT::check(" << s << ") :" << sol<<"\n";
    return sol;
  }

  // HM5_2
  HM5_2::HM5_2(MUSEnumOptions& mo) : SubProblem(mo) {
    MapNode b1 {"b1", "b1"}; MapNode b2 {"b2", "b2"}; MapNode b3 {"b3", "b3"};
    MapNode b4 {"b4", "b4"}; MapNode b5 {"b5", "b5"}; MapNode b6 {"b6", "b6"};
    leaf_names = {"b1", "b2", "b3", "b4", "b5", "b6"};

    if(mo.subproblem_structure == STR_FLAT) {
      tree.children = {b1,b2,b3,b4,b5,b6};
    } else {
      MapNode c12 {"12", {b1, b2}}; MapNode c34 {"34", {b3, b4}}; MapNode c56 {"56", {b5, b6}};
      MapNode c3456 {"3456", {c34, c56}};
      tree.children = {c12, c3456};
    }
  }

  void HM5_2::printSol(const Selection& b) {
    set<string> leaves = getLeaves(b);
    for(const string& leaf : leaves) {
      std::cout << leaf << ", ";
    }
    std::cout << "\n";
  }

  bool HM5_2::check(const Selection& s) {
    vector<bool> b(6, false);

    set<string> leaves = getLeaves(s);
    for(const string& leaf : leaves) {
      if(leaf == "b1") b[0] = true;
      if(leaf == "b2") b[1] = true;
      if(leaf == "b3") b[2] = true;
      if(leaf == "b4") b[3] = true;
      if(leaf == "b5") b[4] = true;
      if(leaf == "b6") b[5] = true;
    }

    bool sol = !((b[0] && b[1]) || (b[0] && b[2]) ||  (b[4] && b[5]) || (b[1] && b[2] && b[5]));
    //sol = !(b[0] &&b[1] &&b[2] &&b[3] &&b[4] &&b[5]);

    if(mopts.verbose_subsolve)
      std::cout << "HM5::check(" << s << ") :" << sol<<"\n";
    return sol;
  }

};



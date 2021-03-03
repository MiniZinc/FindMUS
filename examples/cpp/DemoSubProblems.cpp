#include <algorithm>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "DemoSubProblems.h"
#include "string_utils.h"

namespace HierMUS {
using std::set;
using std::string;
using std::vector;

// HM5
HM5::HM5(MUSEnumOptions &mo) : SubProblem(mo) {
  MapNode b1{"b1", "b1"};
  MapNode b2{"b2", "b2"};
  MapNode b3{"b3", "b3"};
  MapNode b4{"b4", "b4"};
  MapNode b5{"b5", "b5"};
  MapNode b6{"b6", "b6"};
  leaf_names = {"b1", "b2", "b3", "b4", "b5", "b6"};

  if (mo.subproblem_structure == STR_FLAT) {
    tree.children = {b1, b2, b3, b4, b5, b6};
  } else {
    MapNode c12{"12", vector<MapNode>({b1, b2})};
    MapNode c34{"34", vector<MapNode>({b3, b4})};
    MapNode c56{"56", vector<MapNode>({b5, b6})};
    MapNode c1234{"1234", vector<MapNode>({c12, c34})};
    tree.children = {c1234, c56};
  }
}

void HM5::printSol(const Selection &b) {
  set<string> leaves = b.getLeaves();
  for (const string &leaf : leaves) {
    std::cout << leaf << ", ";
  }
  std::cout << "\n";
}

bool HM5::check(const Selection &s) {
  vector<bool> b(6, false);

  set<string> leaves = s.getLeaves();
  for (const string &leaf : leaves) {
    if (leaf == "b1")
      b[0] = true;
    if (leaf == "b2")
      b[1] = true;
    if (leaf == "b3")
      b[2] = true;
    if (leaf == "b4")
      b[3] = true;
    if (leaf == "b5")
      b[4] = true;
    if (leaf == "b6")
      b[5] = true;
  }

  bool sol = !((b[0] && b[1]) ||        // b1 b2
               (b[0] && b[2]) ||        // b1 b3
               (b[4] && b[5]) ||        // b5 b6
               (b[1] && b[2] && b[5])); // b2 b3 b6
  // sol = !(b[0] &&b[1] &&b[2] &&b[3] &&b[4] &&b[5]);

  if (mopts.verbose_subsolve)
    std::cout << "HM5::check(" << s << ") :" << sol << "\n";
  return sol;
}

// FFLAT
FFLAT::FFLAT(MUSEnumOptions &mo) : SubProblem(mo) {
  int n = 16;
  // int n = 8;
  std::vector<MapNode> nodes;

  for (int i = 15; i < 15 + n; i++) {
    // for(int i=7; i<7+n; i++) {
    std::stringstream ss;
    ss << i;
    std::string name = ss.str();
    nodes.push_back(MapNode{name, name});
  }

  tree.children = nodes;
  if (mopts.subproblem_binarize == BIN_ALL) {
    tree.makeBinary([](const MapNode &n) { return n.children.size() > 2; });
  }
}

void FFLAT::printSol(const Selection &b) {
  set<string> leaves = b.getLeaves();
  for (const string &leaf : leaves) {
    std::cout << leaf << ", ";
  }
  std::cout << "\n";
}

bool FFLAT::check(const Selection &s) {
  set<string> leaves = s.getLeaves();
  vector<set<string>> unsat_sets;
  unsat_sets.push_back({"15", "16"});
  unsat_sets.push_back({"16", "17"});
  unsat_sets.push_back({"17", "18"});
  // unsat_sets.push_back({ "7", "8" });
  // unsat_sets.push_back({ "8", "9" });
  // unsat_sets.push_back({ "9", "10" });

  bool sol = true;
  for (set<string> &us : unsat_sets) {
    if (std::includes(leaves.begin(), leaves.end(), us.begin(), us.end()))
      sol = false;
  }

  if (mopts.verbose_subsolve)
    std::cout << "FFLAT::check(" << s << ") :" << sol << "\n";
  return sol;
}

// Ghoulomb (Test case for map_qx)
GLM::GLM(MUSEnumOptions &mo) : SubProblem(mo) {
  MapNode ill1{"ill1", "ill1"};
  MapNode ill2{"ill2", "ill2"};
  MapNode ill3{"ill3", "ill3"};
  MapNode ill4{"ill4", "ill4"};
  MapNode ill5{"ill5", "ill5"};
  MapNode cumu{"cumu", "cumu"};

  leaf_names = {"ill1", "ill2", "ill3", "ill4", "cumu", "ill5"};

  if (mo.subproblem_structure == STR_FLAT) {
    tree.children = {ill1, ill2, ill3, ill4, ill5, cumu};
  } else {
    MapNode ills{"ills", vector<MapNode>({ill1, ill2, ill3, ill4})};
    MapNode top{"top", vector<MapNode>({ills, cumu, ill5})};

    tree.children = {top};
  }
  if (mopts.subproblem_binarize != BIN_NONE) {
    tree.makeBinary([](const MapNode &n) { return n.children.size() > 2; });
  }
}

void GLM::printSol(const Selection &b) {
  set<string> leaves = b.getLeaves();
  for (const string &leaf : leaves) {
    std::cout << leaf << ", ";
  }
  std::cout << "\n";
}

bool GLM::check(const Selection &s) {

  bool sol = true;
  set<string> leaves = s.getLeaves();
  for (const string &leaf : leaves) {
    if (leaf == "cumu")
      sol = false;
  }

  if (mopts.verbose_subsolve)
    std::cout << "GLM::check(" << s << ") :" << sol << "\n";
  return sol;
}

// HM5_2
HM5_2::HM5_2(MUSEnumOptions &mo) : SubProblem(mo) {
  MapNode b1{"b1", "b1"};
  MapNode b2{"b2", "b2"};
  MapNode b3{"b3", "b3"};
  MapNode b4{"b4", "b4"};
  MapNode b5{"b5", "b5"};
  MapNode b6{"b6", "b6"};
  leaf_names = {"b1", "b2", "b3", "b4", "b5", "b6"};

  if (mo.subproblem_structure == STR_FLAT) {
    tree.children = {b1, b2, b3, b4, b5, b6};
  } else {
    MapNode c12{"12", vector<MapNode>({b1, b2})};
    MapNode c34{"34", vector<MapNode>({b3, b4})};
    MapNode c56{"56", vector<MapNode>({b5, b6})};
    MapNode c3456{"3456", vector<MapNode>({c34, c56})};
    tree.children = {c12, c3456};
  }
}

void HM5_2::printSol(const Selection &b) {
  set<string> leaves = b.getLeaves();
  for (const string &leaf : leaves) {
    std::cout << leaf << ", ";
  }
  std::cout << "\n";
}

bool HM5_2::check(const Selection &s) {
  vector<bool> b(6, false);

  set<string> leaves = s.getLeaves();
  for (const string &leaf : leaves) {
    if (leaf == "b1")
      b[0] = true;
    if (leaf == "b2")
      b[1] = true;
    if (leaf == "b3")
      b[2] = true;
    if (leaf == "b4")
      b[3] = true;
    if (leaf == "b5")
      b[4] = true;
    if (leaf == "b6")
      b[5] = true;
  }

  bool sol = !((b[0] && b[1]) || (b[0] && b[2]) || (b[4] && b[5]) ||
               (b[1] && b[2] && b[5]));
  // sol = !(b[0] &&b[1] &&b[2] &&b[3] &&b[4] &&b[5]);

  if (mopts.verbose_subsolve)
    std::cout << "HM5::check(" << s << ") :" << sol << "\n";
  return sol;
}

// Path
Path::Path(MUSEnumOptions &mo) : SubProblem(mo) {
  MapNode a1{"a1", "a1"};
  MapNode a2{"a2", "a2"};
  MapNode a3{"a3", "a3"};
  MapNode b1{"b1", "b1"};
  MapNode b2{"b2", "b2"};
  MapNode b3{"b3", "b3"};
  MapNode c1{"c1", "c1"};
  MapNode c2{"c2", "c2"};
  MapNode c3{"c3", "c3"};

  leaf_names = {"a1", "a2", "a3", "b1", "b2", "b3", "c1", "c2", "c3"};

  if (mo.subproblem_structure == STR_FLAT) {
    tree.children = {a1, a2, a3, b1, b2, b3, c1, c2, c3};
  } else {
    MapNode as{"as", {a1, a2, a3}};
    MapNode bs{"bs", {b1, b2, b3}};
    MapNode cs{"cs", {c1, c2, c3}};

    tree.children = {as, bs, cs};
  }
  if (mopts.subproblem_binarize != BIN_NONE) {
    tree.makeBinary([](const MapNode &n) { return n.children.size() > 2; });
  }
}

void Path::printSol(const Selection &b) {
  set<string> leaves = b.getLeaves();
  for (const string &leaf : leaves) {
    std::cout << leaf << ", ";
  }
  std::cout << "\n";
}

bool Path::check(const Selection &s) {
  bool a1 = false;
  bool a2 = false;
  bool a3 = false;
  bool b1 = false;
  bool b2 = false;
  bool b3 = false;
  bool c1 = false;
  bool c2 = false;
  bool c3 = false;

  bool sol = true;
  set<string> leaves = s.getLeaves();
  for (const string &leaf : leaves) {
    if (leaf == "a1")
      a1 = true;
    if (leaf == "a2")
      a2 = true;
    if (leaf == "a3")
      a3 = true;
    if (leaf == "b1")
      b1 = true;
    if (leaf == "b2")
      b2 = true;
    if (leaf == "b3")
      b3 = true;
    if (leaf == "c1")
      c1 = true;
    if (leaf == "c2")
      c2 = true;
    if (leaf == "c3")
      c3 = true;
  }

  if (a1 && a2)
    sol = false;
  if (c2 && c3)
    sol = false;
  if (c1 && c2)
    sol = false;
  if (a1 && a3)
    sol = false;

  if (mopts.verbose_subsolve)
    std::cout << "Path::check(" << s << ") :" << sol << "\n";
  return sol;
}

// P1f
P1f::P1f(MUSEnumOptions &mo) : SubProblem(mo) {
  MapNode b21{"21", "21"};
  MapNode b22{"22", "22"};
  MapNode b23{"23", "23"};
  MapNode b24{"24", "24"};
  MapNode b25{"25", "25"};
  MapNode b31{"31", "31"};
  MapNode b32{"32", "32"};
  MapNode b33{"33", "33"};
  MapNode b27{"27", "27"};
  MapNode b14{"14", "14"};

  leaf_names = {"21", "22", "23", "24", "25", "31", "32", "33", "27", "14"};

  if (mo.subproblem_structure == STR_FLAT) {
    tree.children = {b21, b22, b23, b24, b25, b31, b32, b33, b27, b14};
  } else {
    MapNode b11{"11", {b21, b22}};
    MapNode b12{"12", {b23, b24, b25}};
    MapNode b13{"13", {MapNode{"26", {b31, b32, b33}}, b27}};

    tree.children = {b11, b12, b13, b14};
  }
  if (mopts.subproblem_binarize != BIN_NONE) {
    tree.makeBinary([](const MapNode &n) { return n.children.size() > 2; });
  }
}

void P1f::printSol(const Selection &b) {
  set<string> leaves = b.getLeaves();
  for (const string &leaf : leaves) {
    std::cout << leaf << ", ";
  }
  std::cout << "\n";
}

bool P1f::check(const Selection &s) {
  bool sol = true;
  set<string> leaves = s.getLeaves();
  for (const string &leaf : leaves) {
    if (leaf == "23")
      sol = false;
    if (leaf == "24")
      sol = false;
    if (leaf == "25")
      sol = false;
    if (leaf == "27")
      sol = false;
    if (leaf == "14")
      sol = false;
  }

  if (mopts.verbose_subsolve)
    std::cout << "P1f::check(" << s << ") :" << sol << "\n";
  return sol;
}

void makeRandomBinary(std::default_random_engine &rand_gen, MapNode &node) {
  if (node.children.size() > 2) {
    std::normal_distribution<> rand_pivot{node.children.size() / 2.0, 2};
    int pivot = static_cast<int>(std::round(rand_pivot(rand_gen)));
    if (pivot > 0 && pivot < node.children.size() - 1) {
      vector<MapNode> left(node.children.begin(),
                           node.children.begin() + pivot);
      vector<MapNode> right(node.children.begin() + pivot, node.children.end());
      node.children.clear();
      node.children.push_back(MapNode(node.path + "_L", {}, left));
      node.children.push_back(MapNode(node.path + "_R", {}, right));
    }
  }
  for (MapNode &child : node.children) {
    makeRandomBinary(rand_gen, child);
  }
}

// RandomProblem
RandomProblem::RandomProblem(MUSEnumOptions &mo, int seed, unsigned int ncons,
                             unsigned int nmuses, unsigned int mussize)
    : SubProblem(mo) {

  std::default_random_engine rand_generator(seed);
  std::uniform_int_distribution<int> rand_ints(0, ncons - 1);

  vector<MapNode> nodes;
  for (unsigned int i = 0; i < ncons; i++) {
    leaf_names.push_back(std::to_string(i));
    nodes.push_back(MapNode{leaf_names.back(), leaf_names.back()});
  }

  tree.children = nodes;
  if (mo.subproblem_structure != STR_FLAT) {
    makeRandomBinary(rand_generator, tree);
  }

  for (unsigned int i = 0; i < nmuses; i++) {
    set<string> mus;

    while (mus.size() < mussize) {
      mus.insert(leaf_names[rand_ints(rand_generator)]);
    }

    if (muses.find(mus) != muses.end()) {
      i--;
    } else {
      muses.insert(mus);
    }
  }
}

void RandomProblem::printSol(const Selection &b) {
  set<string> leaves = b.getLeaves();
  for (const string &leaf : leaves) {
    std::cout << leaf << ", ";
  }
  std::cout << "\n";
}

inline bool is_subset(const set<string> &mus, const set<string> &sel) {
  for (const string &s : mus) {
    if (sel.find(s) == sel.end())
      return false;
  }
  return true;
}

bool RandomProblem::check(const Selection &s) {
  bool sol = true;
  if (!s.empty()) {
    set<string> leaves = s.getLeaves();
    for (const set<string> &mus : muses) {
      if (is_subset(mus, leaves)) {
        sol = false;
        break;
      }
    }
  }
  if (mopts.verbose_subsolve)
    std::cout << "RandomProblem::check(" << s << ") :" << sol << "\n";
  return sol;
}
} // namespace HierMUS

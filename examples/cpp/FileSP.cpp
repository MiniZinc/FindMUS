#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "FileSP.h"
#include "string_utils.h"

namespace HierMUS {
using std::set;
using std::string;
using std::vector;

void FileSP::init(std::istream &is) {
  vector<MapNode> nodes;
  set<string> has_parent;

  for (string line; std::getline(is, line);) {
    std::istringstream entry(line);
    string cmd;
    entry >> cmd;
    if (cmd == "names") {
      // populate leaf_names
      string name;
      while (entry >> name || !entry.eof()) {
        leaf_names.push_back(name);
        nodes.emplace_back(name, name);
      }
    } else if (cmd == "parent" && mopts.subproblem_structure != STR_FLAT) {
      string parent_name;
      entry >> parent_name;
      // Get MapNode for parent (create it if it doesn't exist)
      int pi = -1;
      for (int i = 0; i < nodes.size(); i++) {
        if (nodes[i].path == parent_name) {
          pi = i;
          break;
        }
      }
      if (pi == -1) {
        pi = static_cast<int>(nodes.size());
        nodes.emplace_back(parent_name);
      }
      string child_name;
      while (entry >> child_name || !entry.eof()) {
        int ci = -1;
        for (int i = 0; i < nodes.size(); i++) {
          if (nodes[i].path == child_name) {
            ci = i;
            break;
          }
        }
        if (ci == -1) {
          ci = static_cast<int>(nodes.size());
          nodes.emplace_back(child_name);
        }
        nodes[pi].children.push_back(nodes[ci]);
        has_parent.insert(nodes[ci].path);
      }
    } else if (cmd == "mus") {
      set<string> mus;
      string name;
      while (entry >> name || !entry.eof()) {
        mus.insert(name);
      }
      muses.push_back(mus);
      vector<string> mus_vec(mus.begin(), mus.end());
      oracle.add_set(mus_vec);
    }
  }

  if (mopts.subproblem_structure == STR_FLAT) {
    for (MapNode &node : nodes) {
      if (find(leaf_names.begin(), leaf_names.end(), node.path) !=
          leaf_names.end()) {
        tree.children.push_back(node);
      }
    }
  } else {
    for (MapNode &node : nodes) {
      if (has_parent.find(node.path) == has_parent.end()) {
        tree.children.push_back(node);
      }
    }
  }
  if (mopts.subproblem_binarize != BIN_NONE) {
    tree.makeBinary([](const MapNode &n) { return n.children.size() > 2; });
  }
}

FileSP::FileSP(MUSEnumOptions &mo, std::istream &is)
    : SubProblem(mo), oracle("root", false) {
  init(is);
}

FileSP::FileSP(MUSEnumOptions &mo, string file_path)
    : SubProblem(mo), oracle("root", false) {
  std::ifstream is(file_path);
  if (!is.is_open()) {
    std::cerr << "Can't open file" << file_path << "\n";
    exit(EXIT_FAILURE);
  }
  init(is);
}

std::string FileSP::getSolStr(const Selection &b) {
  std::stringstream ss;
  set<string> leaves = b.getLeaves();
  for (const string &leaf : leaves) {
    ss << leaf << ", ";
  }
  ss << "\n";
  return ss.str();
}

bool FileSP::check(const Selection &s) {
  bool sol = true;
  if (!s.empty()) {
    set<string> leaves = s.getLeaves();
    vector<string> leaves_vec(leaves.begin(), leaves.end());
    sol = !oracle.contains_subset(leaves_vec);
  }
  if (mopts.verbose_subsolve)
    std::cout << "FileSP::check(" << s << ") :" << sol << "\n";

  return sol;
}

bool FileSP::isMUS(const Selection &s) {
  if (!s.empty()) {
    set<string> leaves = s.getLeaves();
    if (find(muses.begin(), muses.end(), leaves) != muses.end())
      return true;
  }
  return false;
}

} // namespace HierMUS

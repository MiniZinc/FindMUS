#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <random>

#include "FileSP.h"
#include "string_utils.h"

namespace HierMUS {
  using std::vector;
  using std::string;
  using std::set;

  inline
  bool is_subset(const set<string>& mus, const set<string>& sel) {
    for(const string& s : mus) {
      if(sel.find(s) == sel.end()) return false;
    }
    return true;
  }

  FileSP::FileSP(MUSEnumOptions& mo, string file_path) : SubProblem(mo) {
    std::ifstream is (file_path);
    if(!is.is_open()) {
      std::cerr << "Can't open file" << file_path << "\n";
      exit(EXIT_FAILURE);
    }
    vector<MapNode> nodes;
    set<string> has_parent;

    for(string line; std::getline(is, line); ) {
      std::istringstream entry(line);
      string cmd;
      entry >> cmd;
      if(cmd == "names") {
        // populate leaf_names
        string name;
        while(entry >> name || !entry.eof()) {
          leaf_names.push_back(name);
          nodes.emplace_back(name, name);
        }
      } else if(cmd == "parent" && mo.subproblem_structure != STR_FLAT) {
        string parent_name;
        entry >> parent_name;
        // Get MapNode for parent (create it if it doesn't exist)
        int pi = -1;
        for(int i=0 ; i<nodes.size(); i++) {
          if(nodes[i].con_id == parent_name) { pi = i; }
        }
        if(pi == -1) {
          pi = nodes.size();
          nodes.emplace_back(parent_name);
        }
        string child_name;
        while(entry >> child_name || !entry.eof()) {
          int ci = -1;
          for(int i=0; i<nodes.size(); i++) {
            if(nodes[i].con_id == child_name) { ci = i; break; }
          }
          assert(ci != -1);
          nodes[pi].children.push_back(nodes[ci]);
          has_parent.insert(nodes[ci].con_id);
        }
      } else if(cmd == "mus") {
        set<string> mus;
        string name;
        while(entry >> name || !entry.eof()) {
          mus.insert(name);
        }
        muses.push_back(mus);
      }
    }

    if(mo.subproblem_structure == STR_FLAT) {
      for(MapNode& node : nodes) {
        if(find(leaf_names.begin(), leaf_names.end(), node.con_id) != leaf_names.end()) {
          tree.children.push_back(node);
        }
      }
    } else {
      for(MapNode& node : nodes) {
        if(has_parent.find(node.con_id) == has_parent.end()) {
          tree.children.push_back(node);
        }
      }
    }
    if(mopts.subproblem_binarize != BIN_NONE) {
      tree.makeBinary([](const MapNode& n){ 
        return n.children.size() > 2;
      });
    }
  }

  void FileSP::printSol(const Selection& b) {
    set<string> leaves = getLeaves(b);
    for(const string& leaf : leaves) {
      std::cout << leaf << ", ";
    }
    std::cout << "\n";
  }

  bool FileSP::check(const Selection& s) {
    bool sol = true;
    if(!s.selected.empty()) {
      set<string> leaves = getLeaves(s);
      for(const set<string>& mus : muses) {
        if(is_subset(mus, leaves)) {
          sol = false;
          break;
        }
      }
    }
    if(mopts.verbose_subsolve)
      std::cout << "FileSP::check(" << s << ") :" << sol<<"\n";
    return sol;
  }

}



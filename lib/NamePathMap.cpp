#include <iostream>
#include <fstream>
#include <string>
#include "NamePathMap.h"

namespace HierMUS {
  using std::string;
  using std::vector;
  using std::ifstream;

  NamePathMap::NamePathMap(void) : has_pathfile{false} {}

  NamePathMap::NamePathMap(const string& pathfilepath) : has_pathfile{true} {
    if(pathfilepath.empty()) {
      has_pathfile = false;
      return;
    }

    ifstream pathstream(pathfilepath);
    if(!pathstream.is_open()) {
      std::cerr << "Error:\tFailed to open pathfile: " << pathfilepath << "\n";
      exit(EXIT_FAILURE);
    }

    string line;
    while(std::getline(pathstream, line)) {
      vector<string> entry = utils::split(line, '\t', true);
      if(isdigit(entry[0][0])) {
        string leaf_name = entry[0];
        leaf_names.push_back(leaf_name);
        if(entry.size()==3)
          nameToPath[leaf_name] = entry[2];
        else
          nameToPath[leaf_name] = "NOPATH";
      }
    }
  }
}

#include "SetTrie.h"
#include <algorithm>

namespace HierMUS {
  using std::string;
  using std::vector;

  void SetTrie::add_set(const vector<string>& set, int i) {
    if(i == set.size()) return;
    const string& val = set[i];
    bool term = i == set.size()-1;

    for(SetTrie& child : children) {
      if(child.value == val) {
        if(term) child.terminal = term;
        else child.add_set(set, i+1);
        return;
      }
    }
    children.emplace_back(val, term);
    if(!term) {
      children.back().add_set(set, i+1);
    }
  }

  bool SetTrie::contains_subset(const vector<string>& set, int i) {
    if(i == set.size()) return false;
    const string& val = set[i];
    bool found = false;

    for(SetTrie& child : children) {
      if(child.value == val) {
        found = child.terminal ? true : child.contains_subset(set, i+1);
        break;
      }
    }
    return found ? true : contains_subset(set, i+1);
  }

  void SetTrie::add_set(const vector<string>& set) {
    vector<string> sorted_set (set.begin(), set.end());
    std::sort(sorted_set.begin(), sorted_set.end());
    // std::cout << "Add set: {" << utils::join(sorted_set, ", ") << "}\n";
    add_set(sorted_set, 0);
  }

  bool SetTrie::contains_subset(const vector<string>& set) {
    vector<string> sorted_set (set.begin(), set.end());
    std::sort(sorted_set.begin(), sorted_set.end());
    bool b = contains_subset(sorted_set, 0);
    //std::cout << "contains_subset: {" << utils::join(sorted_set, ", ") << "} = " << b << "\n";
    return b;
  }

}

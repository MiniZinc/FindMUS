#include "SetTrie.h"
#include <algorithm>

#include <iostream>
#include "string_utils.h"

namespace HierMUS {
  using std::string;
  using std::vector;

  void printSetTrie(SetTrie* s, int depth = 0) {

    std::cout << string(depth, ' ') << s->value  << ": " << s->terminal << "\n";
    for (SetTrie& s1 : s->children) {
      printSetTrie(&s1, depth + 2);
    }
    
  }
  void SetTrie::add_set(const vector<string>& set) {
    vector<string> sorted_set (set.begin(), set.end());
    std::sort(sorted_set.begin(), sorted_set.end());
    //std::cout << "Add set: {" << utils::join(sorted_set, ", ") << "}\n";

    SetTrie* curr = this;
    for (int i = 0; i < sorted_set.size(); i++) {
      string& val = sorted_set[i];
      bool found = false;
      bool is_term = i == sorted_set.size() - 1;
      for (int j = 0; j < curr->children.size(); j++) {
        if (curr->children[j].value == val) {
          found = true;
          curr = &curr->children[j];
          if (curr->terminal) {
            // Early termination. The set is subsumed.
            return;
          }
          if (is_term) {
            curr->terminal = true;
            break;
          }
          break;
        }
      }
      if (!found) {
        curr->children.emplace_back(val, is_term);
        curr = &curr->children.back();
      }
      found = false;
    }
  }

  struct ContainsEntry {
    SetTrie* curr;
    int s_ind;
    ContainsEntry(SetTrie* c, int s) : curr{ c }, s_ind{ s } {}
  };

  bool SetTrie::contains_subset(const vector<string>& set) {
    vector<string> sorted_set (set.begin(), set.end());
    std::sort(sorted_set.begin(), sorted_set.end());

    //std::cout << "\n\nChecking set: {" << utils::join(sorted_set, ", ") << "}\n";
    //std::cout << "Are there subsets here:\n";
    //printSetTrie(this);

    vector<ContainsEntry> stack;
    stack.emplace_back(this, 0);

    while (!stack.empty()) {
      ContainsEntry e = stack.back();
      stack.pop_back();
      //std::cout << "Just popped: " << e.curr->value << ", " << e.s_ind << "\n";

      // Invariant: sorted_set[e.s_ind] == e.curr.value

      if (e.s_ind == sorted_set.size()) {
        //std::cout << "e.s_ind too far\n";
        continue;
      }

      for (int s_rev = sorted_set.size()-1; s_rev >= e.s_ind; s_rev--) {
        //std::cout << "s_rev:" << s_rev << " (" << sorted_set[s_rev] << ")\n";

        for (int c_int = 0; c_int < e.curr->children.size(); c_int++) {
          //std::cout << "    c_int:" << c_int << " (" << e.curr->children[c_int].value << ")\n";
          if (sorted_set[s_rev] == e.curr->children[c_int].value) {
            if (e.curr->children[c_int].terminal) {
              //std::cout << "        Returning true\n";
              return true;
            }
            //std::cout << "        emplace_back(" << e.curr->children[c_int].value << ", " << s_rev << ")\n";
            stack.emplace_back(&e.curr->children[c_int], s_rev);

          }
        }
      }
    }

    //std::cout << "Returning false\n";

    return false;
  }

}

#include "SetTrie.h"
#include <algorithm>

#include "string_utils.h"
#include <iostream>

namespace HierMUS {
using std::string;
using std::vector;

void printSetTrie(SetTrie *s, int depth = 0) {
  std::cout << string(depth, ' ') << s->value << ": " << s->terminal << "\n";
  for (SetTrie &s1 : s->children) {
    printSetTrie(&s1, depth + 2);
  }
}

void SetTrie::add_set(const vector<string> &set) {
  vector<string> sorted_set(set.begin(), set.end());
  std::sort(sorted_set.begin(), sorted_set.end());

  SetTrie *curr = this;
  for (int i = 0; i < sorted_set.size(); i++) {
    string &val = sorted_set[i];
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
  SetTrie *curr;
  int s_ind;
  int c_ind;

  ContainsEntry(SetTrie *c, int si, int ci) : curr{c}, s_ind{si}, c_ind{ci} {}
};

bool SetTrie::contains_subset(const vector<string> &set) {
  vector<string> sorted_set(set.begin(), set.end());
  std::sort(sorted_set.begin(), sorted_set.end());

  vector<ContainsEntry> stack;
  stack.emplace_back(this, 0, 0);

  while (!stack.empty()) {
    ContainsEntry e = stack.back();
    stack.pop_back();

    if (e.s_ind == sorted_set.size())
      continue;
    stack.emplace_back(e.curr, e.s_ind + 1, 0);

    string &val = sorted_set[e.s_ind];
    for (; e.c_ind < e.curr->children.size(); e.c_ind++) {
      if (e.curr->children[e.c_ind].value == val) {
        if (e.curr->children[e.c_ind].terminal) {
          return true;
        }

        stack.emplace_back(e.curr, e.s_ind, e.c_ind + 1);
        stack.emplace_back(&e.curr->children[e.c_ind], e.s_ind + 1, 0);
        break;
      }
    }
  }

  return false;
}

// bool SetTrie::contains_subset(const vector<string>& set) {
//  return contains_subset(set, 0);
//}
// bool SetTrie::contains_subset(const vector<string>& set, int i) {
//  if(i == set.size()) return false;
//  const string& val = set[i];
//  bool found = false;
//  for(SetTrie& child : children) {
//    if(child.value == val) {
//      found = child.terminal ? true : child.contains_subset(set, i+1);
//      break;
//    }
//  }
//  return found ? true : contains_subset(set, i+1);
//}

} // namespace HierMUS

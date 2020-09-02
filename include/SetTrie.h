#ifndef __HIERMUS_SETTRIE_H_
#define __HIERMUS_SETTRIE_H_

#include <string>
#include <vector>

namespace HierMUS {

  class SetTrie {
    public:
      bool terminal;
      std::string value;
      std::vector<SetTrie> children;

      SetTrie(std::string v, bool t) : value{v}, terminal{t} {}

      void add_set(const std::vector<std::string>& set);
      bool contains_subset(const std::vector<std::string>& set);
  };

}

#endif

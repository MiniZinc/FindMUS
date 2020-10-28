#ifndef __NamePathmap_H_
#define __NamePathmap_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "path_utils.h"
#include "string_utils.h"

namespace HierMUS {
class NamePathMap {
private:
  std::unordered_map<std::string, std::string> nameToPath;
  std::vector<std::string> leaf_names;
  bool has_pathfile;

public:
  NamePathMap(void);
  NamePathMap(const std::string &pathfilepath);

  inline const std::string &getPath(const std::string &name) const {
    auto it = nameToPath.find(name);
    if (it != nameToPath.end()) {
      return it->second;
    }
    return name;
  }

  inline const std::string &getPath(size_t i) {
    return nameToPath.at(getName(i));
  }

  inline const std::string &getName(size_t i) {
    if (leaf_names.size() < i) {
      return leaf_names[i];
    }

    // TODO: NCons should typically be known earlier
    if (leaf_names.size() < i + 1)
      leaf_names.resize(static_cast<int>(1.5 * (i + 1)));

    if (leaf_names[i].empty()) {
      leaf_names[i] = std::to_string(i);
    }
    return leaf_names[i];
  }

  inline bool hasNCons(void) const { return has_pathfile; }

  inline const size_t getNCons(void) const { return leaf_names.size(); }
};
} // namespace HierMUS
#endif

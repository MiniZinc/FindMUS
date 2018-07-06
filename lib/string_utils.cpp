#include <sstream>
#include <iterator>

#include "string_utils.h"

namespace utils {
  using std::vector;
  using std::string;

  vector<string> split(const string& str, char delim, bool include_empty) {
    std::stringstream ss;
    ss.str(str);
    std::string item;

    vector<string> result;

    auto inserter = std::back_inserter(result);

    while (std::getline(ss, item, delim)) {
      if(!item.empty() || include_empty)
        *(inserter++) = item;
    }

    return result;
  }

  string join(const vector<string>& strs, const string& sep) {
    std::stringstream ss;
    for(size_t i=0; i<strs.size(); i++) {
      if(i) ss << sep;
      ss << strs[i];
    }
    return ss.str();
  }

}

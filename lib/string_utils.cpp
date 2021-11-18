#include <iterator>
#include <sstream>

#include "string_utils.h"

namespace utils {
using std::string;
using std::stringstream;
using std::vector;

vector<string> split(const string &str, char delim, bool include_empty) {
  std::stringstream ss;
  ss.str(str);
  std::string item;

  vector<string> result;

  auto inserter = std::back_inserter(result);

  while (std::getline(ss, item, delim)) {
    if (!item.empty() || include_empty)
      *(inserter++) = item;
  }

  return result;
}

string join(const vector<string> &strs, const string &sep) {
  std::stringstream ss;
  for (size_t i = 0; i < strs.size(); i++) {
    if (i)
      ss << sep;
    ss << strs[i];
  }
  return ss.str();
}

string escape(const string &orig, const string &repchars, const vector<string> &repstrs) {
  stringstream out;
  size_t last = 0;
  size_t found = orig.find_first_of(repchars);
  while (found != string::npos) {
    out << orig.substr(last, found - last);
    for (size_t i = 0; i < repchars.size(); i++) {
      if (orig[found] == repchars[i]) {
        out << repstrs[i];
        break;
      }
    }
    last = found + 1;
    found = orig.find_first_of(repchars, found + 1);
  }
  out << orig.substr(last);
  return out.str();
}

string escape(const string &orig, bool html, bool nls) {
  string repchars = "\\&\"\'<>";
  vector<string> repstrs;
  if (html)
    repstrs = {"\\", "&amp;", "&quot;", "&apos;", "&lt;", "&gt;"};
  else
    repstrs = {"\\\\", "&", "\\\"", "'", "<", ">"};

  if (nls) {
    repchars += "\n\r";
    repstrs.emplace_back("\\n");
    repstrs.emplace_back("\\r");
  }

  return escape(orig, repchars, repstrs);
}

string escape_nls(const string& orig) {
  string repchars = "\n\r";
  vector<string> repstrs = {"\\n", "\\r"};
  return escape(orig, repchars, repstrs);
}

string escape_text(const string& orig) {
  string repchars = "\\&\"\'<>";
  vector<string> repstrs = {"\\\\", "&", "\\\"", "'", "<", ">"};
  return escape(orig, repchars, repstrs);
}

string escape_html(const string& orig) {
  string repchars = "\\&\"\'<>";
  vector<string> repstrs = {"\\", "&amp;", "&quot;", "&apos;", "&lt;", "&gt;"};
  return escape(orig, repchars, repstrs);
}

} // namespace utils

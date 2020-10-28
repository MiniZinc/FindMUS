#include <sstream>
#include <string>
#include <vector>

#include "path_utils.h"
#include "string_utils.h"

namespace utils {

using std::string;
using std::stringstream;
using std::vector;

vector<string> getAllAssigns(const string &path) {
  static std::regex assignment_regex{reg_mzn_ident "=" reg_number};
  vector<string> assigns;
  auto assignment_begin =
      std::sregex_iterator(path.begin(), path.end(), assignment_regex);
  auto assignment_end = std::sregex_iterator();

  for (std::sregex_iterator i = assignment_begin; i != assignment_end; i++) {
    std::smatch match = *i;
    assigns.push_back(match.str());
  }

  return assigns;
}

string generalizeLabel(const string &path_el, bool remove_locations, bool mix) {
  std::stringstream new_label;

  if (remove_locations) {
    static std::regex index_regex{"\\|" reg_number};
    new_label << std::regex_replace(path_el, index_regex, "|0");
  } else {
    static std::regex generalize_regex{"=" reg_number};
    new_label << std::regex_replace(path_el, generalize_regex, "=$$");
  }

  if (mix)
    new_label << path_el;

  return new_label.str();
}

vector<string> getPathHead(const string &path, bool leaveModel,
                           bool includeTrail) {
  vector<string> pathSplit = utils::split(path, major_sep);

  if (!includeTrail && leaveModel)
    return {pathSplit.back()};

  string mzn_file;
  vector<string> previousHead;

  if (pathSplit.size() == 0)
    return previousHead;

  size_t i = 0;
  do {
    string path_head = pathSplit[i];
    vector<string> head = utils::split(path_head, minor_sep);
    string head_file;
    if (head.size() > 0) {
      if (i == 0)
        mzn_file = head[0];
      head_file = head[0];
    }

    if (!leaveModel && head_file != mzn_file)
      return previousHead;

    if (!includeTrail)
      previousHead.clear();
    previousHead.push_back(path_head);
    i++;
  } while (i < pathSplit.size());

  return previousHead;
}

} // namespace utils

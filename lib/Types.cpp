#include <iostream>
#include <map>

#include "Types.h"
#include "path_utils.h"
#include "string_utils.h"

#include <minizinc/file_utils.hh>

namespace HierMUS {

std::ostream &operator<<(std::ostream &os, Statistics const &s) {
  os << "map: " << std::setw(6) << s.map_calls << "\tsat: " << std::setw(6)
     << s.sat_calls << "\ttotal: " << std::setw(6) << s.sat_calls + s.map_calls;
  return os;
}

void ConstraintInfo::setAnnotatedNamesFrom(const MiniZinc::ConstraintI *ci) {
  std::stringstream explain;
  MiniZinc::Expression *en =
      MiniZinc::get_annotation(MiniZinc::Expression::ann(ci->e()), "mzn_expression_name");
  if (en) {
    MiniZinc::Call *c = MiniZinc::Expression::cast<MiniZinc::Call>(en);
    expression_name = MiniZinc::Expression::cast<MiniZinc::StringLit>(c->arg(0))->v().c_str();
  }
  MiniZinc::Expression *cn =
      MiniZinc::get_annotation(MiniZinc::Expression::ann(ci->e()), "mzn_constraint_name");
  if (cn) {
    MiniZinc::Call *c = MiniZinc::Expression::cast<MiniZinc::Call>(cn);
    constraint_name = MiniZinc::Expression::cast<MiniZinc::StringLit>(c->arg(0))->v().c_str();
  }
}

ConstraintSet::ConstraintSet() {}
void ConstraintSet::addConstraintInfo(const std::string &path,
                                      const ConstraintInfo &ci) {
  constraints[path].push_back(ci);
}

string ConstraintSet::getLongSummary(void) {
  std::stringstream ss;
  ss << getShortSummary();
  ss << "\nTraces:" << std::endl;

  for (auto &ps : constraints) {
    ss << ps.first << std::endl;
  }

  return ss.str();
}

string ConstraintSet::getShortSummary(const string &sep) {
  std::unordered_set<string> names;
  std::unordered_set<string> leaves;
  for (auto &ps : constraints) {
    for (const ConstraintInfo &n : ps.second) {
      leaves.insert(n.leaf_name);
      std::stringstream ss;
      ss << n.name;
      if (!n.constraint_name.empty() || !n.expression_name.empty()) {
        ss << "@{";
        if (!n.constraint_name.empty()) {
          ss << n.constraint_name;
          if (!n.expression_name.empty()) {
            ss << "@" << n.expression_name << "}";
          }
        } else if (!n.expression_name.empty()) {
          ss << n.expression_name << "}";
        }
      }
      ss << ":(" << n.assigns << ")";

      names.insert(ss.str());
    }
  }
  std::vector<string> names_vec(names.begin(), names.end());

  std::sort(names_vec.begin(), names_vec.end());
  vector<string> leaves_vec(leaves.begin(), leaves.end());
  std::stringstream shortSol;

  shortSol << "MUS: " << utils::join(leaves_vec, " ") << sep
           << "Brief: " << utils::join(names_vec, sep) << std::endl;
  return shortSol.str();
}

std::string ConstraintSet::getJSONSummary(void) {
  std::stringstream json;
  json << "{";

  json << "\"constraints\": [" << std::endl;
  unsigned int remain = static_cast<unsigned int>(constraints.size());
  for (auto &pcs : constraints) {
    unsigned int remain_inner = static_cast<unsigned int>(pcs.second.size());
    for (const ConstraintInfo &ci : pcs.second) {
      json << "{" << std::endl;
      json << "\t\"leaf_name\": "
           << "\"" << ci.leaf_name << "\"," << std::endl;
      json << "\t\"name\": "
           << "\"" << utils::escape_text(ci.name) << "\"," << std::endl;
      json << "\t\"path\": "
           << "\"" << utils::escape_text(ci.path) << "\"," << std::endl;
      json << "\t\"assigns\": "
           << "\"" << utils::escape_text(ci.assigns) << "\"," << std::endl;
      json << "\t\"constraint_name\": "
           << "\"" << utils::escape_text(ci.constraint_name) << "\","
           << std::endl;
      json << "\t\"expression_name\": "
           << "\"" << utils::escape_text(ci.expression_name) << "\""
           << std::endl;
      json << "}";
      if (--remain_inner)
        json << "," << std::endl;
    }
    if (--remain)
      json << "," << std::endl;
  }
  json << "]" << std::endl;

  json << "}" << std::endl;
  return json.str();
}

std::string ConstraintSet::getHUMANSummary(const string &sep,
                                           MapDepth map_depth) {
  std::map<string, std::set<string>> names;

  for (auto &pcs : constraints) {
    for (const ConstraintInfo &ci : pcs.second) {
      string con_name =
          (!ci.constraint_name.empty()) ? ci.constraint_name : "Uncategorised";

      std::stringstream name_ss;
      if (map_depth == DEPTH_INSTANCE && !ci.expression_name.empty()) {
        name_ss << ci.expression_name;
      } else if (!ci.path.empty()) {
        string full_path_head =
            utils::getPathHead(ci.path, map_depth == DEPTH_PROGRAM, false)[0];
        vector<string> head_parts = utils::split(full_path_head, '|');
        head_parts[0] = MiniZinc::FileUtils::base_name(head_parts[0]);
        name_ss << utils::join(head_parts, "|");
        if (!ci.assigns.empty())
          name_ss << " ( " << ci.assigns << " )";
      } else {
        name_ss << ci.name << " ( " << ci.leaf_name << " )";
      }

      auto &exp_names = names[con_name];
      exp_names.insert(name_ss.str());
    }
  }

  std::stringstream out;
  out << "The following constraints are incompatible:" << sep;
  for (auto &np : names) {
    out << " " << np.first << ":" << sep;
    for (auto &n : np.second) {
      out << "  - " << n << sep;
    }
  }
  out << sep << std::flush;
  return out.str();
}

std::string ConstraintSet::getHTMLSummary(MapDepth map_depth) {
  std::stringstream ss;
  std::unordered_set<string> paths;
  for (auto &ps : constraints) {
    paths.insert(utils::generalizeLabel(ps.first, false, false));
  }
  std::vector<string> path_vec(paths.begin(), paths.end());

  ss << "<div class=\"explanation\" style=\"border:1px solid black\">"
     << "<a href=\"highlight:?" << utils::join(path_vec, "&")
     << "\">"
     << getHUMANSummary("<br/>", map_depth) << "</a></div>";

  return ss.str();
}

std::string ConstraintSet::getSummary(SubProblemOutputFormat format,
                                      MapDepth map_depth) {
  std::stringstream ss;
  if (format == OUT_NORMAL) {
    ss << getLongSummary();
  } else if (format == OUT_DEBUG) {
    ss << getShortSummary();
  } else if (format == OUT_HTML) {
    ss << getHTMLSummary(map_depth);
  } else if (format == OUT_JSON) {
    ss << getJSONSummary();
  } else if (format == OUT_HUMAN) {
    ss << getHUMANSummary("\n", map_depth);
  }

  return ss.str();
}
} // namespace HierMUS

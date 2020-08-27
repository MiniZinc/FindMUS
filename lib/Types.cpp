#include <iostream>

#include "Types.h"
#include "path_utils.h"

namespace HierMUS {

  std::ostream& operator<<(std::ostream& os, Statistics const& s) {
    os << "map: "     << std::setw(6) << s.map_calls
       << "\tsat: "   << std::setw(6) << s.sat_calls
       << "\ttotal: " << std::setw(6) << s.sat_calls + s.map_calls;
    return os;
  }

  void ConstraintInfo::setAnnotatedNamesFrom(const MiniZinc::ConstraintI* ci) {
    std::stringstream explain;
    MiniZinc::Expression* en = MiniZinc::getAnnotation(ci->e()->ann(), "mzn_expression_name");
    if(en) {
      MiniZinc::Call* c = en->cast<MiniZinc::Call>();
      expression_name = c->arg(0)->cast<MiniZinc::StringLit>()->v().str();
    }
    MiniZinc::Expression* cn = MiniZinc::getAnnotation(ci->e()->ann(), "mzn_constraint_name");
    if(cn) {
      MiniZinc::Call* c = cn->cast<MiniZinc::Call>();
      constraint_name = c->arg(0)->cast<MiniZinc::StringLit>()->v().str();
    }
  }

  ConstraintSet::ConstraintSet() {}
  void ConstraintSet::addConstraintInfo(const std::string& path, const ConstraintInfo& ci) {
    constraints[path].push_back(ci);
  }

  string ConstraintSet::getLongSummary(void) {
    std::stringstream ss;
    ss << getShortSummary();
    ss << "\nTraces:" << std::endl;

    for(auto& ps : constraints) {
      ss << ps.first << std::endl;
    }

    return ss.str();
  }

  string ConstraintSet::getShortSummary(const string& sep) {
    std::unordered_set<string> names;
    std::unordered_set<string> leaves;
    for(auto& ps : constraints) {
      for(const ConstraintInfo& n : ps.second) {
        leaves.insert(n.leaf_name);
        std::stringstream ss;
        ss << n.name;
        if(!n.constraint_name.empty() || !n.expression_name.empty()) {
          ss << "@{";
          if(!n.constraint_name.empty()) {
            ss << n.constraint_name;
            if(!n.expression_name.empty()) {
              ss << "@" << n.expression_name << "}";
            }
          } else if(!n.expression_name.empty()) {
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
    json << "%%%mzn-json-start" << std::endl;
    json << "{";

    json << "\"constraints\": [" << std::endl;
    unsigned int remain = static_cast<unsigned int>(constraints.size());
    for(auto& pcs : constraints) {
      unsigned int remain_inner = static_cast<unsigned int>(pcs.second.size());
      for(const ConstraintInfo& ci : pcs.second) {
        json << "{" << std::endl;
        json << "\t\"leaf_name\": " << "\"" << ci.leaf_name << "\"," << std::endl;
        json << "\t\"name\": " << "\""            << utils::escape(ci.name, false) << "\"," << std::endl;
        json << "\t\"path\": " << "\""            << utils::escape(ci.path, false) << "\"," << std::endl;
        json << "\t\"assigns\": " << "\""         << utils::escape(ci.assigns, false) << "\"," << std::endl;
        json << "\t\"constraint_name\": " << "\"" << utils::escape(ci.constraint_name, false) << "\"," << std::endl;
        json << "\t\"expression_name\": " << "\"" << utils::escape(ci.expression_name, false) << "\"" << std::endl;
        json << "}";
        if(--remain_inner) json << "," << std::endl;
      }
      if(--remain) json << "," << std::endl;
    }
    json << "]" << std::endl;

    json << "}" << std::endl;
    json << "%%%mzn-json-end" << std::endl;
    return json.str();
  }

  std::string ConstraintSet::getHTMLSummary(void) {
    std::stringstream ss;
    std::unordered_set<string> paths;
    for(auto& ps : constraints) {
      paths.insert(utils::generalizeLabel(ps.first, false, false));
    }
    std::vector<string> path_vec(paths.begin(), paths.end());

    ss << "%%%mzn-html-start\n"
       << "<div class=\"explanation\" style=\"border:1px solid black\">"
       << "<a href=\"highlight:?" << utils::join(path_vec, "&") << "\">"
       << getShortSummary("<br/>")
       << "</a></div>\n"
       << "%%%mzn-html-end\n" << std::endl;

    return ss.str();
  }

  std::string ConstraintSet::getSummary(SubProblemOutputFormat format) {
    std::stringstream ss;
    if(format == OUT_NORMAL) {
      ss << getLongSummary();
    } else if(format == OUT_DEBUG) {
      ss << getShortSummary();
    } else if(format == OUT_HTML) {
      ss << getHTMLSummary();
    } else if(format == OUT_JSON) {
      ss << getJSONSummary();
    }

    return ss.str();
  }
}


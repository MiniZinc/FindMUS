#include <iostream>

#include "Types.h"
#include "path_utils.h"

namespace HierMUS {

  void getLeaves(const MapNode& node, std::set<std::string>& leaves) {
    if(!node.children.empty()) {
      for(const MapNode& child : node.children)
        getLeaves(child, leaves);
    } else {
      vector<string> split_leaves = utils::split(node.con_id, '#', false);
      for(const string& l : split_leaves)
        leaves.insert(l);
    }
  }

  std::set<std::string> getLeaves(const Selection& b) {
    std::set<std::string> leaves;
    for(const MapNode* node : b.selected) {
      getLeaves(*node, leaves);
    }
    return leaves;
  }

  inline
  std::string printMapNode(bool pol, const std::string& prefix, const MapNode* mn) {
    std::stringstream ss;
    if(!pol) ss << "~";
    if(!mn->var.isLeaf) ss << prefix;
    ss << mn->path;
    return ss.str();
  }

  std::ostream& streamMapNodeSet(std::ostream& os, std::set<MapNode*> const& mns, bool pol, std::string prefix) {
    bool first = true;
    os << "{";
    for(const MapNode* mn : mns) {
      if(!first) os << ", ";
      os << printMapNode(pol, prefix, mn);
      first = false;
    }
    os << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, std::set<MapNode*> const& mns) {
    return streamMapNodeSet(os, mns, true, "c_");
  }

  std::ostream& operator<<(std::ostream& os, std::set<ExpandedNode> const& inc) {
    bool first = true;
    os << "{";
    for(const ExpandedNode& en : inc) {
      if(!first) os << ", ";
      os << printMapNode(true, "e_", en.child);
      first = false;
    }
    os << "}";
    return os;
  }

  static bool output_leaves_for_selections = false;
  static bool output_details_for_selections = true;

  std::ostream& operator<<(std::ostream& os, Selection const& a) {
    bool first = true;
    if(a.selected.empty()) {
      os << "empty";
      return os;
    }
    if (output_leaves_for_selections)  {
      std::set<std::string> leaves = getLeaves(a);
      for(const string& leaf : leaves) {
        os << (first ? " " : ", ") << leaf;
        first = false;
      }
      return os;
    }
    os << a.selected;
    if(output_details_for_selections) {
      os << " inc: " << a.include;
      os << " exc: ";
      streamMapNodeSet(os, a.exclude, false, "d_");
    }
    return os;
  }


  std::ostream& operator<<(std::ostream& os, Statistics const& s) {
    os << "map: " << s.map_calls;
    os << "\tsat: " << s.sat_calls;
    os << "\ttotal: " << s.sat_calls + s.map_calls;
    return os;
  }

  void debugPrint(Selection& s) {
    std::stringstream ss;

    ss << "<s: {";
    bool first = true;
    for(const MapNode* mn : s.selected) {
      ss << (first ? " " : ", ")
        << (mn->var.isLeaf ? "" : "c_")
        << mn->path;
      first = false;
    }
    ss << " }, ";

    ss << "in: {";
    first = true;
    for(const ExpandedNode& en : s.include) {
      ss << (first ? " " : ", ")
        << (en.child->var.isLeaf ? "" : "e_")
        << en.child->path;
      first = false;
    }
    ss << " }, ";

    ss << "ex: {";
    first = true;
    for(const MapNode* mn : s.exclude) {
      ss << (first ? " " : ", ")
        << (mn->var.isLeaf ? "" : "e_")
        << mn->path;
      first = false;
    }
    ss << " }>";
    std::cout << ss.str();
  }

  bool isLeaves(const Selection& s) {
    for(const ExpandedNode& en : s.include) if(!en.child->var.isLeaf) return false;
    return true;
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
    ss << "\nTraces:\n";

    for(auto& ps : constraints) {
      ss << ps.first << "\n";
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
             << "Brief: " << utils::join(names_vec, sep) << "\n";
    return shortSol.str();
  }

  std::string ConstraintSet::getJSONSummary(void) {
    std::stringstream json;
    json << "%%%mzn-json-start\n";
    json << "{";

    json << "\"constraints\": [\n";
    unsigned int remain = static_cast<unsigned int>(constraints.size());
    for(auto& pcs : constraints) {
      unsigned int remain_inner = static_cast<unsigned int>(pcs.second.size());
      for(const ConstraintInfo& ci : pcs.second) {
        json << "{\n";
        json << "\t\"leaf_name\": " << "\"" << ci.leaf_name << "\",\n";
        json << "\t\"name\": " << "\"" << ci.name << "\",\n";
        json << "\t\"path\": " << "\"" << ci.path << "\",\n";
        json << "\t\"assigns\": " << "\"" << ci.assigns << "\",\n";
        json << "\t\"constraint_name\": " << "\"" << ci.constraint_name << "\",\n";
        json << "\t\"expression_name\": " << "\"" << ci.expression_name << "\"\n";
        json << "}";
        if(--remain_inner) json << ",\n";
      }
      if(--remain) json << ",\n";
    }
    json << "]\n";

    json << "}\n";
    json << "%%%mzn-json-end\n";
    return json.str();
  }

  std::string ConstraintSet::getHTMLSummary(void) {
    std::stringstream ss;
    std::unordered_set<string> paths;
    for(auto& ps : constraints) {
      paths.insert(utils::generalizeLabel(ps.first, false));
    }
    std::vector<string> path_vec(paths.begin(), paths.end());

    ss << "%%%mzn-html-start\n"
       << "<div class=\"explanation\" style=\"border:1px solid black\">"
       << "<a href=\"highlight:?" << utils::join(path_vec, "&") << "\">"
       << getShortSummary("<br/>")
       << "</a></div>\n"
       << "%%%mzn-html-end\n\n";

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


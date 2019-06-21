#include <chuffed/core/options.h>
#include <chuffed/core/engine.h>
#include <chuffed/flatzinc/flatzinc.h>
#include <chuffed/vars/modelling.h>

#include <vector>
#include <string>
#include <iomanip>
#include <limits>

#include "ChuffedSubsetMap.h"
#include "string_utils.h"
#include "path_utils.h"

namespace HierMUS {
  using std::vector;
  using std::string;

  void ChuffedSubsetProblem::setMaximal(bool max_mode) {
    if(max_mode) {
      for(int i=0; i<leaves_top; i++) {
        leaves[i].setPreferredVal(PV_MAX);
      }
    } else {
      for(int i=0; i<leaves_top; i++) {
        leaves[i].setPreferredVal(mopts.getRandBool() ? PV_MIN : PV_MAX);
      }
    }
  }

  void ChuffedSubsetProblem::pushTempBlockSupersets(const Selection& selection) {
    bool alreadyEnabled = tempBlocking;
    if(!alreadyEnabled) { enableTempBlocking(); }
    blockSupersets(selection);
    if(!alreadyEnabled) { disableTempBlocking(); }
  }

  void ChuffedSubsetProblem::pushTempBlockSubsets(const Selection& selection) {
    bool alreadyEnabled = tempBlocking;
    if(!alreadyEnabled) { enableTempBlocking(); }
    blockSubsets(selection);
    if(!alreadyEnabled) { disableTempBlocking(); }
  }

  void ChuffedSubsetProblem::popTempBlock(void) {
    vec<Lit> cl;
    cl.push(~tempStack.back());
    tempStack.pop_back();
    sat.addClause(cl);
  }

  ChuffedSubsetProblem::ChuffedSubsetProblem(SubProblem* prob, MUSEnumOptions& mo)
    : SubsetMap{prob, mo}, leaves_top{0}, branches_top{0}, obj{nullptr} {
      // This is required for chuffed to work since FlatZinc::s is a static space
      FlatZinc::s = nullptr;

      double build_start = wallClockTime();
      const MapNode& tree = prob->getTree();
      counts nc = tree.getCounts();

      createVars(leaves, nc.nleaves);
      createVars(conjs,  nc.nbranches);
      createVars(disjs,  nc.nbranches);
      createVars(eqs,    nc.nbranches);

      root = addConnections(tree);

      leaves.resize( leaves_top );
      if(branches_top > 0) {
        conjs.resize(branches_top);
        disjs.resize(branches_top);
        eqs  .resize(branches_top);
      }

      vec<Branching*> va;
      //  // Don't branch on leaves first?
      //  for(int i=0; i<branches_top; i++) {
      //    va.push(new BoolView(conjs[i]));
      //  }
      //

      vec<Lit> blockRoot;
      for(int i=0; i<leaves_top; i++) {
        va.push(new BoolView(leaves[i]));
        blockRoot.push(~leaves[i].getLit(true));
      }
      output_vars(va);
      branch(va, VAR_INORDER, VAL_MAX);

      //sat.addClause(blockRoot);

      // Add sat brancher
      so.vsids = true;
      engine.branching->add(&sat);


      engine.start_time = wallClockTime();
      engine.opt_time = 0;
      engine.search_time = 0;
      engine.base_memory = 0;
      engine.conflicts = 0;
      engine.nodes = 0;
      engine.propagations = 0;
      engine.solutions = 0;
      engine.next_simp_db = 0;
      engine.output_stream = &null_stream;

      so.nof_solutions = 1;
      so.time_out = 86400;
      so.branch_random = true;
      so.print_sol = true;
      engine.problem = this;

      setMaximal(true);

      if(mopts.verbose_map) {
        std::cout << "SubsetMap:\tmap loaded:\ttime:\t" << std::fixed <<  std::setprecision(5) << wallClockTime() - build_start << "\n";
        std::cout << "SubsetMap:\tnleaves:\t" << leafNodes.size() << "\tnbranches:\t" << branchNodes.size() << "\n";
      }
      std::cout << "SubsetMap:\tnleaves:\t" << leafNodes.size() << "\tnbranches:\t" << branchNodes.size() << "\n";
    }

  void ChuffedSubsetProblem::reset(void) {
    while(!tempStack.empty()) {
      popTempBlock();
    }
  }

  inline
  string getFilenameFromNode(const MapNode& node) {
    if(node.path == "all") return "";
    vector<string> split_path = utils::split(node.path, major_sep, false);
    if(split_path.empty()) return "";
    vector<string> first_path = utils::split(split_path.front(), minor_sep, false);
    if(first_path.size() == 1) return "";
    return first_path.front();
  }

  bool isBoundary(const MapNode& node) {
    if(node.children.empty()) return true;
    string filename = getFilenameFromNode(node);
    if(filename.empty()) return false;

    for(const MapNode& child : node.children) {
      if(filename == getFilenameFromNode(child)) { return false; }
    }

    return true;
  }

  MapNode ChuffedSubsetProblem::addConnections(const MapNode& node, unsigned int depth) {
    if( (mopts.map_depth == DEPTH_INSTANCE && isBoundary(node)) ||
        (mopts.map_depth == DEPTH_CUSTOM   && mopts.map_depth_max == depth)) {
      BoolView* var = &leaves[leaves_top];
      leaves_top++;
      leafNodes.emplace_back(node.path, node.con_id, HierVar(var));
      MapNode& special_node = leafNodes.back();
      special_node.var.isLeaf = true;
      for(const MapNode& pn : node.children)
        special_node.children.push_back(pn);
      return special_node;  // construct leaf node
    }
    if(node.children.size() == 0) {
      BoolView* var = &leaves[leaves_top];
      leaves_top++;
      leafNodes.emplace_back(node.path, node.con_id, HierVar(var));
      return MapNode(leafNodes.back());  // construct leaf node
    } else {
      vec<BoolView> conj;
      vec<BoolView> disj;

      int branch_idx = branches_top;
      branches_top++;

      vector<MapNode> children;
      for(const MapNode& n : node.children) {
        MapNode cd = addConnections(n, depth + 1);
        children.push_back(cd);

        if(cd.var.isLeaf) {
          conj.push(*cd.var.leaf);
          disj.push(*cd.var.leaf);
        } else {
          conj.push(*cd.var.conj);
          disj.push(*cd.var.disj);
        }
      }

      MapNode* this_cd;
      branchNodes.emplace_back(node.path, HierVar(&conjs[branch_idx], &disjs[branch_idx], &eqs[branch_idx]));
      this_cd = &branchNodes.back();
      this_cd->children = children;

      // Don't add the links
      array_bool_and(conj, *this_cd->var.conj);
      array_bool_or(disj, *this_cd->var.disj);
      bool_rel(*this_cd->var.conj, BRT_EQ_REIF, *this_cd->var.disj, *this_cd->var.eq);

      return *this_cd;
    }
  }

  bool simplifyVecLit(vec<Lit>& ps) {
    int i, j;
    for (i = j = 0; i < ps.size(); i++) {
      if (sat.value(ps[i]) == l_True) {
        return false;
      }
      if (sat.value(ps[i]) == l_Undef) {
        ps[j++] = ps[i];
      }
    }
    ps.resize(j);
    return ps.size() > 0;
  }

  Selection ChuffedSubsetProblem::expand(const Selection& s) {
    Selection newSel;
    newSel.selected = s.selected;
    newSel.exclude = s.exclude;

    if(mopts.verbose_map) std::cout << "SubsetMap:\tExpanding: " << s.include;
    for(const ExpandedNode& en : s.include) {
      if(s.selected.empty() || s.selected.find(en.parent) != s.selected.end()) {
        MapNode* nm = en.child;
        if(nm->var.isLeaf || nm->children.empty()) {
          newSel.include.insert(en);
        } else {
          for(MapNode& child : nm->children)
            newSel.include.insert(ExpandedNode(en.parent, &child));
        }

      } else {
        newSel.exclude.insert(en.child);
      }
    }
    if(mopts.verbose_map) std::cout << " to " << newSel.include << "\n";

    return newSel;
  }

  void ChuffedSubsetProblem::block(vec<Lit>& blockClause) {
    if(simplifyVecLit(blockClause)) {
      sat.addClause(blockClause);
    } else {
      if(blockClause.size() == 0) {
        consistent = false;
      }
    }
  }

  void ChuffedSubsetProblem::blockSupersets(const Selection& selection) {
    vec<Lit> blockClause;
    if(mopts.verbose_map) std::cout << "SubsetMap:\tSuperset block: " << (tempBlocking ? "<Temp> " : "") ;

    for(const MapNode* node : selection.selected) {
      blockClause.push(node->var.isLeaf ? ~node->var.leaf->getLit(true) : ~node->var.conj->getLit(true));
    }
    if(mopts.verbose_map) {
      streamMapNodeSet(std::cout, selection.selected, false, "c_");
      std::cout << "\n";
    }

    if(tempBlocking) {
      int control = sat.newVar();
      Lit conLit = Lit(control, true);
      tempStack.push_back(conLit);
      sat.polarity[control] = false;
      blockClause.push(~conLit);
    }

    block(blockClause);
  }

  void ChuffedSubsetProblem::blockSubsets(const Selection& selection) {
    vec<Lit> blockClause;
    if(mopts.verbose_map) std::cout << "SubsetMap:\tSubset block:   " << (tempBlocking ? "<Temp> " : "") ;

    for(const MapNode* node : selection.exclude) {
      blockClause.push( node->var.isLeaf ? node->var.leaf->getLit(true) : node->var.disj->getLit(true)  );
    }
    if(mopts.verbose_map) {
      streamMapNodeSet(std::cout, selection.exclude, true, "d_");
      std::cout << "\n";
    }

    if(tempBlocking) {
      int control = sat.newVar();
      Lit conLit = Lit(control, true);
      tempStack.push_back(conLit);
      sat.polarity[control] = false;
      blockClause.push(~conLit);
    }

    block(blockClause);
  }

  void ChuffedSubsetProblem::print(std::ostream&) {
    solution_set = {};
    solution_set.exclude = solution_template.exclude;

    for(const ExpandedNode& en : solution_template.include) {
      if ((en.child->var.isLeaf && en.child->var.leaf->getVal()) || en.child->var.conj->getVal()) {
        solution_set.selected.insert(en.child);
        solution_set.exclude.erase(en.child);
        solution_set.include.insert(ExpandedNode(en.child));  // New Selection so parent can be discarded
      } else {
        solution_set.exclude.insert(en.child);
      }
    }
  }

  Selection ChuffedSubsetProblem::getRootSelector() {
    return {{&root}, {ExpandedNode(&root)}, {}};
  }

  Selection ChuffedSubsetProblem::getLeavesSelector() {
    Selection s_temp;
    for(size_t i=0; i<leafNodes.size(); i++)
      s_temp.include.insert(ExpandedNode(&leafNodes[i]));  // which nodes to get solutions for
    return s_temp;
  }

  Selection ChuffedSubsetProblem::getSelection() {
    return getSelection(getLeavesSelector());
  }

  Selection ChuffedSubsetProblem::getSelection(const Selection& selection) {
    if(!consistent) {
      if(mopts.verbose_map) { std::cout << "SubsetMap:\tInconsistent. Returning: {}. \n"; }
      return {};  // Return empty Selection
    }
    solution_template = selection;

    vec<Lit> atLeastOne;

    if(mopts.verbose_map) std::cout << "SubsetMap:\tgetSelection("<< selection <<")\t{" << (engine.opt_var ? "maximal" : "any") <<"}\twith assumptions: {";
    vec<BoolView> all_assumptions;
    for(const ExpandedNode& enode : selection.include) {
      if(!enode.child->var.isLeaf) {  // Don't force leaves to be active
        all_assumptions.push(enode.child->var.eq->getLit(true));
        if(mopts.verbose_map) std::cout << " e" << enode.child->path;
      }
      atLeastOne.push(enode.child->var.isLeaf ? enode.child->var.leaf->getLit(true) : enode.child->var.conj->getLit(true));
    }

    int control = sat.newVar();
    Lit conLit = Lit(control, true);
    sat.polarity[control] = false;
    atLeastOne.push(~conLit);
    for(MapNode* node : forceInclude) {
      solution_template.exclude.erase(node);
      all_assumptions.push(node->var.isLeaf ? node->var.leaf->getLit(true) : node->var.conj->getLit(true));
      if(mopts.verbose_map) std::cout << (node->var.isLeaf ? " " :  " c") << node->path;
    }

    sat.addClause(atLeastOne);
    all_assumptions.push(conLit);

    for(Lit& l : tempStack) {
      all_assumptions.push(l);
    }

    for(MapNode* node : solution_template.exclude) {
      all_assumptions.push(node->var.isLeaf ? ~node->var.leaf->getLit(true) : ~node->var.disj->getLit(true));
      if(mopts.verbose_map) std::cout << (node->var.isLeaf ? " ~" :  " ~d") << node->path;
    }

    if(mopts.verbose_map) std::cout << " }\n";
    engine.set_assumptions(all_assumptions);

    solution_set = {};
    so.time_out = 86400;
    engine.best_sol = -1;
    double start_time = wallClockTime();
    engine.solutions = 0;
    engine.solve(this, "hierMUS");
    sat.btToLevel(0);
    sat.simplifyDB();

    if(mopts.verbose_map) { std::cout << "SubsetMap:\tSolve took " << std::fixed << std::setprecision(5) << wallClockTime() - start_time << " seconds. Returning: " << solution_set << ". \n"; }

    return solution_set;
  }

}

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

  ChuffedSubsetProblem::ChuffedSubsetProblem(SubProblem* prob, MUSEnumOptions& mo)
    : SubsetMap{prob, mo}, leaves_top{0}, branches_top{0}, obj{NULL}, consistent{true} {
      FlatZinc::s = NULL;

      double build_start = wallClockTime();
      const MapNode& tree = prob->getTree();
      counts nc = tree.getCounts();

      createVars(leaves, nc.nleaves);
      createVars(conjs,  nc.nbranches);
      createVars(disjs,  nc.nbranches);
      createVars(eqs,    nc.nbranches);

      root = addConnections(tree);

      leaves.shrink( leaves_top - 1);
      if(branches_top > 0) {
        conjs.shrink(branches_top - 1);
        disjs.shrink(branches_top - 1);
        eqs  .shrink(branches_top - 1);
      }

      // Change this to adjustable brancher later
      vec<Branching*> va;
      if(mopts.map_enumeration_alg == ALG_STACKMUS) {
        for(int i=0; i<branches_top; i++) {
          va.push(new BoolView(conjs[i]));
          output_var( &conjs[i]);
        }
      }
      for(int i=0; i<leaves_top; i++) {
        va.push(new BoolView(leaves[i]));
        output_var( &leaves[i]);
      }
      branch(va, VAR_INORDER, VAL_MAX);

      // Add sat brancher
      // so.vsids = true;
      // engine.branching->add(&sat);

      addObjective();

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

      so.time_out = 86400;
      so.branch_random = true;
      so.print_sol = true;
      engine.problem = this;

      if(mopts.verbose_map) {
        std::cout << "SubsetMap:\tmap loaded:\ttime:\t" << std::fixed <<  std::setprecision(5) << wallClockTime() - build_start << "\n";
        std::cout << "SubsetMap:\tnleaves:\t" << leafNodes.size() << "\tnbranches:\t" << branchNodes.size() << "\n";
      }
      std::cout << "SubsetMap:\tnleaves:\t" << leafNodes.size() << "\tnbranches:\t" << branchNodes.size() << "\n";
    }

  void ChuffedSubsetProblem::addObjective() {
    createVar(obj, 0, leaves.size(), true);
    bool_linear(leaves, IRT_EQ, obj);
    optimize(obj, OPT_MAX);
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

  MapNode ChuffedSubsetProblem::addConnections(const MapNode& node, int depth) {
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
      if (sat.value(ps[i]) == l_True) return false;
      if (sat.value(ps[i]) == l_Undef) ps[j++] = ps[i];
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
      consistent = false;
    }
  }

  void ChuffedSubsetProblem::blockSupersets(const Selection& selection) {
    vec<Lit> blockClause;
    if(mopts.verbose_map) std::cout << "SubsetMap:\tSuperset block: ";

    for(const MapNode* node : selection.selected) {
      blockClause.push( node->var.isLeaf ? ~node->var.leaf->getLit(true) : ~node->var.conj->getLit(true)  );
    }
    if(mopts.verbose_map) {
      streamMapNodeSet(std::cout, selection.selected, false, "c_");
      std::cout << "\n";
    }

    block(blockClause);
  }

  void ChuffedSubsetProblem::blockSubsets  (const Selection& selection) {
    vec<Lit> blockClause;
    if(mopts.verbose_map) std::cout << "SubsetMap:\tSubset block: ";

    for(const ExpandedNode& enode : selection.include) {
      const MapNode* node = enode.child;
      blockClause.push( node->var.isLeaf ? ~node->var.leaf->getLit(true) : ~node->var.disj->getLit(true)  );
    }
    for(const MapNode* node : selection.exclude) {
      blockClause.push( node->var.isLeaf ? node->var.leaf->getLit(true) : node->var.disj->getLit(true)  );
    }
    if(mopts.verbose_map) {
      streamExpandedNodeSet(std::cout, selection.include, false, "d_");
      std::cout << " + ";
      streamMapNodeSet(std::cout, selection.exclude, true, "d_");
      std::cout << "\n";
    }

    block(blockClause);
  }

  void ChuffedSubsetProblem::print(std::ostream& os) {
    solution_set = {};
    solution_set.exclude = solution_template.exclude;

    for(const ExpandedNode& en : solution_template.include) {
      if ((en.child->var.isLeaf && en.child->var.leaf->getVal()) || en.child->var.conj->getVal()) {
        solution_set.selected.insert(en.child);
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
    for(int i=0; i<leafNodes.size(); i++)
      s_temp.include.insert(ExpandedNode(&leafNodes[i]));  // which nodes to get solutions for
    return s_temp;
  }

  Selection ChuffedSubsetProblem::getSelection() {
    return getSelection(getLeavesSelector());
  }

  Selection ChuffedSubsetProblem::getSelection(const Selection& selection) {

    if(!consistent) return {};  // Return empty Selection
    solution_template = selection;

    if(mopts.verbose_map) std::cout << "SubsetMap:\tgetSelection("<< selection <<")\twith assumptions: {";
    vec<BoolView> all_assumptions;
    for(const ExpandedNode& enode : selection.include) {
      if(!enode.child->var.isLeaf) {  // Don't force leaves to be active
        all_assumptions.push(enode.child->var.eq->getLit(true));
        if(mopts.verbose_map) std::cout << " e" << enode.child->path;
      }
    }

    for(const MapNode* node : selection.exclude) {
      all_assumptions.push(node->var.isLeaf ? ~node->var.leaf->getLit(true) : ~node->var.disj->getLit(true));
      if(mopts.verbose_map) std::cout << (node->var.isLeaf ? " ~" :  " ~d") << node->path;
    }
    if(mopts.verbose_map) std::cout << " }\n";
    engine.set_assumptions(all_assumptions);

    solution_set = {};
    so.time_out = 86400;
    engine.best_sol = -1;
    double start_time = wallClockTime();
    engine.solve(this, "hierMUS");
    sat.btToLevel(0);
    sat.simplifyDB();

    if(mopts.verbose_map) { std::cout << "SubsetMap:\tSolve took " << std::fixed << std::setprecision(5) << wallClockTime() - start_time << " seconds. Returning: " << solution_set << ". \n"; }

    return solution_set;
  }

}

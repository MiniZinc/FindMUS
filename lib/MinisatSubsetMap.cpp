#include <minisat/core/Solver.h>

#include <vector>
#include <string>
#include <iomanip>
#include <limits>

#include "MinisatSubsetMap.h"
#include "string_utils.h"
#include "path_utils.h"

namespace HierMUS {
  using Minisat::mkLit;
  using Minisat::lbool;

  using std::vector;
  using std::string;

  void MinisatSubsetMap::setMaximal(bool max_mode) {
    if(max_mode) {
      for(int i=0; i<leaves_top; i++) {
        solver.setPolarity(leaves[i], Minisat::l_False);
      }
    } else {
      for(int i=0; i<leaves_top; i++) {
        solver.setPolarity(leaves[i], Minisat::l_Undef);
      }
      solver.rnd_pol = true;
    }
  }

  void MinisatSubsetMap::pushTempBlockSupersets(const Selection& selection) {
    bool alreadyEnabled = tempBlocking;
    if(!alreadyEnabled) { enableTempBlocking(); }
    blockSupersets(selection);
    if(!alreadyEnabled) { disableTempBlocking(); }
  }

  void MinisatSubsetMap::pushTempBlockSubsets(const Selection& selection) {
    bool alreadyEnabled = tempBlocking;
    if(!alreadyEnabled) { enableTempBlocking(); }
    blockSubsets(selection);
    if(!alreadyEnabled) { disableTempBlocking(); }
  }

  void MinisatSubsetMap::popTempBlock(void) {
    solver.addClause(~tempStack.back());
    tempStack.pop_back();
  }

  void createVars(Minisat::Solver& solver, Minisat::vec<Minisat::Var>& arr, size_t nvars) {
    for(int i=0; i<nvars; i++) arr.push(solver.newVar(Minisat::l_False, true));
  }

  MinisatSubsetMap::MinisatSubsetMap(SubProblem* prob, MUSEnumOptions& mo)
    : SubsetMap{prob, mo}, leaves_top{0}, branches_top{0} {
      std::chrono::time_point<std::chrono::system_clock> build_start = std::chrono::system_clock::now();
      const MapNode& tree = prob->getTree();
      counts nc = tree.getCounts();

      createVars(solver, leaves, nc.nleaves);
      createVars(solver, conjs,  nc.nbranches);
      createVars(solver, disjs,  nc.nbranches);
      createVars(solver, eqs,    nc.nbranches);

      root = addConnections(tree);

      leaves.shrink( leaves_top );
      if(branches_top > 0) {
        conjs.shrink(branches_top);
        disjs.shrink(branches_top);
        eqs  .shrink(branches_top);
      }

      //Minisat::vec<Minisat::Lit> blockRoot;
      //for(int i=0; i<leaves_top; i++) {
      //  blockRoot.push(~mkLit(leaves[i]));
      //}
      //solver.addClause(blockRoot);

      setMaximal(true);

      if(mopts.verbose_map) {
        std::chrono::duration<double> dur = std::chrono::system_clock::now() - build_start;
        std::cout << "SubsetMap:\tmap loaded:\ttime:\t" << std::fixed << std::setprecision(5) << dur.count() << "\n";
        std::cout << "SubsetMap:\tnleaves:\t" << leafNodes.size() << "\tnbranches:\t" << branchNodes.size() << "\n";
      }
    }

  void MinisatSubsetMap::reset(void) {
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

  MapNode MinisatSubsetMap::addConnections(const MapNode& node, unsigned int depth) {
    if( (mopts.map_depth == DEPTH_INSTANCE && isBoundary(node)) ||
        (mopts.map_depth == DEPTH_CUSTOM   && mopts.map_depth_max == depth)) {
      Minisat::Var* var = &leaves[leaves_top];
      leaves_top++;
      leafNodes.emplace_back(node.path, node.con_id, HierVar(*var));
      MapNode& special_node = leafNodes.back();
      special_node.var.isLeaf = true;
      for(const MapNode& pn : node.children)
        special_node.children.push_back(pn);
      return special_node;  // construct leaf node
    }
    if(node.children.size() == 0) {
      Minisat::Var* var = &leaves[leaves_top];
      leaves_top++;
      leafNodes.emplace_back(node.path, node.con_id, HierVar(*var));
      return MapNode(leafNodes.back());  // construct leaf node
    } else {
      Minisat::vec<Minisat::Var> conj;
      Minisat::vec<Minisat::Var> disj;

      int branch_idx = branches_top;
      branches_top++;

      vector<MapNode> children;
      for(const MapNode& n : node.children) {
        MapNode cd = addConnections(n, depth + 1);
        children.push_back(cd);

        if(cd.var.isLeaf) {
          conj.push(cd.var.leaf);
          disj.push(cd.var.leaf);
        } else {
          conj.push(cd.var.conj);
          disj.push(cd.var.disj);
        }
      }

      MapNode* this_cd;
      branchNodes.emplace_back(node.path, HierVar(conjs[branch_idx], disjs[branch_idx], eqs[branch_idx]));
      this_cd = &branchNodes.back();
      this_cd->children = children;

      // Don't add the links
      // array_bool_and(conj, *this_cd->var.conj);
      // array_bool_and(as, r) = {r, ~*as} /\ forall(a in as)({a,~r})
      {
        Minisat::vec<Minisat::Lit> control_cl;
        control_cl.push(mkLit(this_cd->var.conj));
        for(int i=0; i<conj.size(); i++) {
          Minisat::Var v = conj[i];
          control_cl.push(~mkLit(v));
          solver.addClause(~mkLit(this_cd->var.conj), mkLit(v));
        }
        solver.addClause(control_cl);
      }

      // array_bool_or(disj, *this_cd->var.disj);
      // array_bool_or(as, r)  = {*as, ~r} /\ forall(a in as)({r, ~a})
      {
        Minisat::vec<Minisat::Lit> control_cl;
        control_cl.push(~mkLit(this_cd->var.disj));
        for(int i=0; i<disj.size(); i++) {
          Minisat::Var v = disj[i];
          control_cl.push(mkLit(v));
          solver.addClause(mkLit(this_cd->var.disj), ~mkLit(v));
        }
        solver.addClause(control_cl);
      }

      // r -> (p /\ q) = {~r, ~p, q} /\ {~r, p, ~q}
      solver.addClause(~mkLit(this_cd->var.eq), ~mkLit(this_cd->var.conj),  mkLit(this_cd->var.disj));
      solver.addClause(~mkLit(this_cd->var.eq),  mkLit(this_cd->var.conj), ~mkLit(this_cd->var.disj));

      return *this_cd;
    }
  }

  Selection MinisatSubsetMap::expand(const Selection& s, const Selection& m) {
    Selection newSel;
    newSel.selected = s.selected;
    newSel.exclude = s.exclude;
    newSel.is_min = s.is_min;

    if(mopts.verbose_map) std::cout << "SubsetMap:\tExpanding: " << s.include << " from " << m.include;
    for(const ExpandedNode& en : s.include) {
      if (std::any_of(m.include.begin(),
                      m.include.end(),
                      [&](const ExpandedNode& m_en) {return m_en.child == en.child; })) {
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
      } else {
        newSel.include.insert(en);
      }
    }
    if(mopts.verbose_map) std::cout << " to " << newSel.include << "\n";

    return newSel;
  }

  void MinisatSubsetMap::block(Minisat::vec<Minisat::Lit>& blockClause) {
    solver.addClause(blockClause);
  }

  void MinisatSubsetMap::blockSupersets(const Selection& selection) {
    Minisat::vec<Minisat::Lit> blockClause;

    if(mopts.verbose_map) std::cout << "SubsetMap:\tSuperset block: " << (tempBlocking ? "<Temp> " : "") ;

    for(const MapNode* node : selection.selected) {
      blockClause.push(node->var.isLeaf ? ~mkLit(node->var.leaf) : ~mkLit(node->var.conj));
    }
    if(mopts.verbose_map) {
      streamMapNodeSet(std::cout, selection.selected, false, "c_");
      std::cout << "\n";
    }

    if(tempBlocking) {
      int control = solver.newVar(Minisat::l_Undef, false);
      tempStack.push_back(mkLit(control));
      blockClause.push(~mkLit(control));
    }

    block(blockClause);
  }

  void MinisatSubsetMap::blockSubsets(const Selection& selection) {
    Minisat::vec<Minisat::Lit> blockClause;
    if(mopts.verbose_map) std::cout << "SubsetMap:\tSubset block:   " << (tempBlocking ? "<Temp> " : "") ;

    for(const MapNode* node : selection.exclude) {
      blockClause.push( node->var.isLeaf ? mkLit(node->var.leaf) : mkLit(node->var.disj) );
    }
    if(mopts.verbose_map) {
      streamMapNodeSet(std::cout, selection.exclude, true, "d_");
      std::cout << "\n";
    }

    if(tempBlocking) {
      int control = solver.newVar(Minisat::l_Undef, false);
      tempStack.push_back(mkLit(control));
      blockClause.push(~mkLit(control));
    }

    block(blockClause);
  }

  Selection MinisatSubsetMap::getRootSelector() {
    return {{&root}, {ExpandedNode(&root)}, {}};
  }

  Selection MinisatSubsetMap::getLeavesSelector() {
    Selection s_temp;
    for(size_t i=0; i<leafNodes.size(); i++)
      s_temp.include.insert(ExpandedNode(&leafNodes[i]));  // which nodes to get solutions for
    return s_temp;
  }

  Selection MinisatSubsetMap::getSelection() {
    return getSelection(getLeavesSelector(), true);
  }

  Selection MinisatSubsetMap::getSelection(const Selection& selection, bool blockSat /* = true */ ) {
    if(!consistent) {
      if(mopts.verbose_map) { std::cout << "SubsetMap:\tInconsistent. Returning: {}. \n"; }
      return {};  // Return empty Selection
    }
    solution_template = selection;

    for(int i=0; i< conjs.size(); i++) { solver.setDecisionVar( conjs[i], false); }
    for(int i=0; i< disjs.size(); i++) { solver.setDecisionVar( disjs[i], false); }
    for(int i=0; i<   eqs.size(); i++) { solver.setDecisionVar(   eqs[i], false); }
    for(int i=0; i<leaves.size(); i++) { solver.setDecisionVar(leaves[i], false); }

    Minisat::vec<Minisat::Lit> atLeastOne;
    if(mopts.verbose_map) std::cout << "SubsetMap:\tgetSelection("<< selection <<")\twith assumptions: {";
    Minisat::vec<Minisat::Lit> all_assumptions;
    for(const ExpandedNode& enode : selection.include) {
      if(!enode.child->var.isLeaf) {  // Don't force leaves to be active
        all_assumptions.push(mkLit(enode.child->var.eq));
        if(mopts.verbose_map) std::cout << " e" << enode.child->path;
      }
      Minisat::Var v = enode.child->var.isLeaf ? enode.child->var.leaf : enode.child->var.conj;
      atLeastOne.push(mkLit(v));
      solver.setDecisionVar(v, true);
    }

    Minisat::Var control = solver.newVar(Minisat::l_Undef, false);
    Minisat::Lit conLit = mkLit(control);
    atLeastOne.push(~conLit);
    for(MapNode* node : forceInclude) {
      solution_template.exclude.erase(node);
      all_assumptions.push(node->var.isLeaf ? mkLit(node->var.leaf) : mkLit(node->var.conj));
      if(mopts.verbose_map) std::cout << (node->var.isLeaf ? " " :  " c") << node->path;
    }

    solver.addClause(atLeastOne);
    all_assumptions.push(conLit);

    for(Minisat::Lit& l : tempStack) {
      all_assumptions.push(l);
    }

    for(MapNode* node : solution_template.exclude) {
      all_assumptions.push(node->var.isLeaf ? ~mkLit(node->var.leaf) : ~mkLit(node->var.disj));
      if(mopts.verbose_map) std::cout << (node->var.isLeaf ? " ~" :  " ~d") << node->path;
    }

    if(mopts.verbose_map) std::cout << " }\n";

    auto start_time = std::chrono::system_clock::now();
    bool r = solver.solve(all_assumptions);

    // std::cout << "\n";
    // std::cout << "nAssigns(): " <<  solver.nVars() << "\n";
    // std::cout << "nVars(): " <<  solver.nVars() << "\n";
    // std::cout << "nClauses(): " <<  solver.nVars() << "\n";
    // std::cout << "nLearnt(): " <<  solver.nVars() << "\n";
    // std::cout << "nFreeVars(): " <<  solver.nVars() << "\n";
    // std::cout << "solver.okay(): " << solver.okay() << "\n";
    // solver.printStats();
    // std::cout << "\n";

    solution_set = {};
    solution_set.exclude = solution_template.exclude;

    if(r) {
      for(const ExpandedNode& en : solution_template.include) {
        if((en.child->var.isLeaf && solver.modelValue(en.child->var.leaf) == Minisat::l_True)
           || solver.modelValue(en.child->var.conj) == Minisat::l_True) {
          solution_set.selected.insert(en.child);
          solution_set.exclude.erase(en.child);
          solution_set.include.insert(ExpandedNode(en.child));  // New Selection so parent can be discarded
        } else {
          solution_set.exclude.insert(en.child);
        }
      }
    }

    solver.addClause(~conLit);
    solver.simplify();

    if(mopts.verbose_map) {
      std::chrono::duration<double> dur = std::chrono::system_clock::now() - start_time;
      std::cout << "SubsetMap:\tSolve took " << std::fixed << std::setprecision(5) << dur.count() << " seconds. Returning: " << solution_set << ". \n"; }

    return solution_set;
  }

}

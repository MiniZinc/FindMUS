#include <vector>
#include <string>
#include <iomanip>
#include <limits>

#include "MinisatSubsetMap.h"
#include "string_utils.h"
#include "path_utils.h"

namespace HierMUS {

  using std::vector;
  using std::string;

  void MinisatSubsetProblem::setMaximal(bool max_mode) { }

  void MinisatSubsetProblem::pushTempBlockSupersets(const Selection& selection) { }

  void MinisatSubsetProblem::pushTempBlockSubsets(const Selection& selection) { }

  void MinisatSubsetProblem::popTempBlock(void) { }

  MinisatSubsetProblem::MinisatSubsetProblem(SubProblem* prob, MUSEnumOptions& mo) : SubsetMap{prob, mo} { }

  void MinisatSubsetProblem::reset(void) { }

  inline string getFilenameFromNode(const MapNode& node) {   }

  bool isBoundary(const MapNode& node) { return true; }

  MapNode MinisatSubsetProblem::addConnections(const MapNode& node, unsigned int depth) { return {}; }

  bool simplifyVecLit(vec<Lit>& ps) { return false; }

  Selection MinisatSubsetProblem::expand(const Selection& s, const Selection& m) { return {}; }

  void MinisatSubsetProblem::block(vec<Lit>& blockClause) { }

  void MinisatSubsetProblem::blockSupersets(const Selection& selection) { }

  void MinisatSubsetProblem::blockSubsets(const Selection& selection) { }

  void MinisatSubsetProblem::print(std::ostream&) { }

  Selection MinisatSubsetProblem::getRootSelector() { return {}; }

  Selection MinisatSubsetProblem::getLeavesSelector() { return {}; }

  Selection MinisatSubsetProblem::getSelection() { return getSelection(getLeavesSelector(), true); }

  Selection MinisatSubsetProblem::getSelection(const Selection& selection, bool blockSat /* = true */ ) { return {}; }

}

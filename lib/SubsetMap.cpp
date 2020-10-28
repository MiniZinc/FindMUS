#include <iomanip>
#include <limits>
#include <string>
#include <vector>

#include "SubsetMap.h"

namespace HierMUS {
using std::string;
using std::vector;

void SubsetMap::clearForceInclude(void) { forceInclude.clear(); }

void SubsetMap::setForceInclude(const Selection &selection) {
  clearForceInclude();
  for (MapNode *node : selection.selected) {
    forceInclude.insert(node);
  }
}

Selection SubsetMap::getRandomSelection(const Selection &selection,
                                        const Selection &finc,
                                        bool strictSubset) {
  setMaximal(false);
  if (strictSubset) {
    pushTempBlockSupersets(selection);
  }
  setForceInclude(finc);
  Selection s = getSelection(selection);
  clearForceInclude();
  if (strictSubset) {
    popTempBlock();
  }
  setMaximal(true);
  return s;
}

} // namespace HierMUS

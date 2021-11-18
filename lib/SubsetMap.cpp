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
  for (const MapNode *node : selection.const_selected()) {
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

void SubsetMap::log(const std::string& msg, unsigned int level) const {
  mopts.log(MUSEnumOptions::LOG_MAP, msg, level);
}

void SubsetMap::error(const std::string& msg) const {
  mopts.error(MUSEnumOptions::LOG_MAP, msg);
}

} // namespace HierMUS

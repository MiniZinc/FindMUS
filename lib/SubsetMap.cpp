#include <chuffed/core/options.h>
#include <chuffed/core/engine.h>
#include <chuffed/flatzinc/flatzinc.h>
#include <chuffed/vars/modelling.h>

#include <vector>
#include <string>
#include <iomanip>
#include <limits>

#include "SubsetMap.h"

namespace HierMUS {
  using std::vector;
  using std::string;

  void SubsetMap::clearForceInclude(void) {
    forceInclude.clear();
  }

  void SubsetMap::setForceInclude(const Selection& selection) {
    clearForceInclude();
    for(MapNode* node : selection.selected) {
      forceInclude.insert(node);
    }
  }

  Selection SubsetMap::getRandomSelection(const Selection& selection, const Selection& finc, bool strictSubset) {
    setMaximal(false);
    if(strictSubset) pushTempSupersetBlock(selection);
    setForceInclude(finc);
    Selection s = getSelection(selection);
    clearForceInclude();
    if(strictSubset) popTempSupersetBlock();
    setMaximal(true);
    return std::move(s);
  }

}

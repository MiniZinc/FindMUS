#include "string_utils.h"
#include "SubProblem.h"

namespace HierMUS {

  SubProblem::SubProblem(MUSEnumOptions& mo) : tree {"all"}, mopts(mo) {}

  SubProblem::~SubProblem() {}

  MapNode& SubProblem::getTree() { return tree; }

}

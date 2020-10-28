#include "SubProblem.h"
#include "string_utils.h"

namespace HierMUS {

SubProblem::SubProblem(MUSEnumOptions &mo) : tree{"all"}, mopts(mo) {}

SubProblem::~SubProblem() {}

MapNode &SubProblem::getTree() { return tree; }

} // namespace HierMUS

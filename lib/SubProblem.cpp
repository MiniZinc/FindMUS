#include "SubProblem.h"
#include "string_utils.h"

namespace HierMUS {

SubProblem::SubProblem(MUSEnumOptions &mo) : tree{"all"}, mopts(mo) {}

SubProblem::~SubProblem() {}

MapNode &SubProblem::getTree() { return tree; }

void SubProblem::log(const std::string& msg, unsigned int level) const {
  mopts.log(MUSEnumOptions::LOG_SOLVE, msg, level);
}

void SubProblem::error(const std::string& msg) const {
  mopts.error(MUSEnumOptions::LOG_SOLVE, msg);
}

} // namespace HierMUS

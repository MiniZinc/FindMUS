#ifndef PATH_UTILS_HH
#define PATH_UTILS_HH

#include <vector>
#include <string>
#include <regex>

#define reg_mzn_ident "[A-Za-z_][A-Za-z0-9_]*"
#define reg_number "[0-9]*"

namespace utils {

#define minor_sep '|'
#define major_sep ';'

std::string generalizeLabel(const std::string& path_el, bool mix);
std::vector<std::string> getAllAssigns(const std::string& path);


std::vector<std::string> getPathHead(const std::string& path,
                                     bool leaveModel = false,
                                     bool includeTrail = false);


}


#endif // PATH_UTILS_HH

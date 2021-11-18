#ifndef STRING_UTILS_HH
#define STRING_UTILS_HH

#include <string>
#include <vector>

namespace utils {
std::vector<std::string> split(const std::string &str, char delim,
                               bool include_empty = false);
std::string join(const std::vector<std::string> &strs, const std::string &sep);
std::string escape(const std::string &orig, const std::string &repchars, const std::vector<std::string> &repstrs);
std::string escape(const std::string &orig, bool html = true, bool nls = false);
std::string escape_nls(const std::string& orig);
std::string escape_text(const std::string& orig);
std::string escape_html(const std::string& orig);
} // namespace utils

#endif // STRING_UTILS_HH

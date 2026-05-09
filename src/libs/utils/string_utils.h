#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
#include <vector>

namespace http_server::strutils {
    std::vector<std::string> split_n(std::string s, const std::string &del, int count);
    std::vector<std::string> split(std::string s, const std::string &del);
    void trim(std::string &str);
}

#endif

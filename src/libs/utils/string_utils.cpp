#include "string_utils.h"
#include <string>
#include <vector>

using namespace std;

vector<string> http_server::strutils::split_n(string s, const string &del, int count) {
  vector<string> tokens;
  size_t pos = s.find(del);
  int i = 0;
  while (pos != string::npos && i != count) {

    tokens.push_back(s.substr(0, pos));
    s.erase(0, pos + del.length());
    pos = s.find(del);
    ++i;
  }

  if (s.size() != 0) {
    tokens.push_back(s);
  }
  return tokens;
}

vector<string> http_server::strutils::split(string s, const string &del){
    return strutils::split_n(std::move(s), del, -1);
}

void http_server::strutils::trim(string &str) {
  const char *whitespaces = " \t\n\r\f\v";
  str.erase(str.find_last_not_of(whitespaces) + 1);
  str.erase(0, str.find_first_not_of(whitespaces));
}
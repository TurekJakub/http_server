#include "parser.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <istream>
#include <iterator>
#include <map>
#include <ostream>
#include <print>
#include <string>
#include <utility>
#include <vector>

using namespace std;

expected<HttpResponse, std::string> HttpParser::parse_response(std::istream &input) {
  std::string header_line;
  getline(input, header_line);
  // getline(input, header_line);
  if (input.fail()) {
    return unexpected("a");
  }
  auto tokens = split(header_line, " ");
  unsigned int status_code = stoi(tokens[1]);
  for (auto &&token : tokens) {
    println("{}", token);
  }
  HttpHeaders headers;
  string line;
  while (getline(input, line)) {
    if (input.fail()) {
      return unexpected("");
    }
    trim(line);
    if (line.size() == 0) {
      break;
    }

    auto tokens = split(line, ":", 1);
    for_each(tokens.begin(), tokens.end(), [this](string &s) { return this->trim(s); });
    /*
    for (auto &&token : tokens) {
      println("{}", token);
      }
      */

    if (tokens.size() != 2) {
      println("{}", line);
      return unexpected("not enough tokens in response header line");
    }

    headers.insert(make_pair(tokens[0], tokens[1]));
  }

  HttpBody body = HttpBody{(std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>()};

  return HttpResponse(headers, body, status_code);
}

std::vector<std::string> HttpParser::split(std::string s, const string &del, int count) {
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

void HttpParser::trim(string &str) {
  const char *whitespaces = " \t\n\r\f\v";
  str.erase(str.find_last_not_of(whitespaces) + 1);
  str.erase(0, str.find_first_not_of(whitespaces));
}

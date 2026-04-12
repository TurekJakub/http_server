#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <expected>
#include <istream>
#include <map>
#include <string>
#include <sys/types.h>
#include <vector>

enum HttpMethod { GET, POST, PUT, DELETE, PATCH };

typedef std::vector<char> HttpBody;
typedef std::map<std::string, std::string> HttpHeaders;

class HttpResponse {
public:
  HttpResponse(HttpHeaders headers, HttpBody body, unsigned short status) : headers(headers), body(body), status(status) {}
  HttpHeaders headers;
  HttpBody body;
  unsigned short status;

private:
};

class HttpParser {
public:
  std::expected<HttpResponse, std::string> parse_response(std::istream &input);
private: 
    std::vector<std::string> split(std::string s, const std::string& del, int count = -1);
    void trim(std::string& str);
  };

#endif
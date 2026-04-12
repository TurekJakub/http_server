#ifndef PARSER_H
#define PARSER_H

#include <expected>
#include <istream>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <sys/types.h>
#include <vector>

enum HttpMethod { GET, POST, PUT, DELETE, PATCH };

typedef std::vector<char> HttpBody;
typedef std::map<std::string, std::string> HttpHeader;

class HttpHeaders {
  public:
  void set (std::string header, std::string value);
  std::optional<std::string> get(std::string header) const;
  std::ranges::subrange<HttpHeader::const_iterator> get_all();
  private:
  std::map<std::string, std::string> headers;
};

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
  std::expected<HttpHeaders, std::string> parse_headers(std::istream &input);
  std::expected<HttpBody,std::string> parse_chunked_body(std::istream &input);
  std::expected<HttpBody,std::string> parse_body(std::istream &input, const HttpHeaders& headers);
};

#endif
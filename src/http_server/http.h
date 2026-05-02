#ifndef PARSER_H
#define PARSER_H

#include <expected>
#include <istream>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <utility>
#include <variant>
#include <vector>

#define unwrap(result)                                                                                                                     \
  if (!result) {                                                                                                                           \
    return unexpected(result.error());                                                                                                     \
  }

inline static constexpr unsigned char TLS_HANDSHAKE_IDENTIFIER_BYTE = 0x16;
inline static std::string HTTP_BODY_DELIMITER = "\r\n\r\n";

struct GET {};
struct POST {};
struct PUT {};
struct DELETE {};
struct PATCH {};

typedef std::variant<GET, POST, PUT, DELETE, PATCH, std::string> HttpMethod;

inline constexpr std::array<std::pair<std::string_view, HttpMethod>, 5> method_map = {
    {{"GET", GET{}}, {"POST", POST{}}, {"PUT", PUT{}}, {"DELETE", DELETE{}}, {"PATCH", PATCH{}}}
};

HttpMethod str_to_method(std::string method_str);

typedef std::vector<char> HttpBody;
typedef std::map<std::string, std::string> HttpHeader;

class HttpHeaders {
public:
  HttpHeaders() = default;
  HttpHeaders(std::map<std::string, std::string> headers) : headers(std::move(headers)) {};
  void set(std::string header, std::string value);
  std::optional<std::string> get(std::string header) const;
  std::ranges::subrange<HttpHeader::const_iterator> get_all();

private:
  std::map<std::string, std::string> headers;
};

class HttpResponse {
public:
  HttpResponse() {};
  HttpResponse(HttpHeaders headers, HttpBody body, unsigned short status) : headers(std::move(headers)), body(body), status(status) {}
  std::string serialize();
  std::expected<void, std::string> serialize(std::ostream &serialize_to);
  HttpHeaders headers;
  HttpBody body;
  unsigned short status;

private:
};

class HttpRequest {
public:
  HttpRequest(HttpMethod method, HttpHeaders headers, HttpBody body, std::string url)
      : method(method), headers(headers), body(body), requested_url(url) {};
  HttpMethod method;
  HttpHeaders headers;
  HttpBody body;
  std::string requested_url;

private:
};

class HttpParser {
public:
  std::expected<HttpResponse, std::string> parse_response(std::istream &input);
  std::expected<HttpRequest, std::string> parse_request(std::istream &input);

private:
  std::expected<HttpHeaders, std::string> parse_headers(std::istream &input);
  std::expected<HttpBody, std::string> parse_chunked_body(std::istream &input);
  std::expected<HttpBody, std::string> parse_body(std::istream &input, const HttpHeaders &headers);
};

std::optional<std::string> http_status_to_reason(unsigned short status_code);
HttpResponse get_redirection_response(std::string target);

#endif
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

namespace http_server::http {
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

enum class HttpStatus : unsigned short;

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
  HttpResponse(HttpHeaders headers, HttpBody body, HttpStatus status) : headers_internal(std::move(headers)), body_internal(body), status_internal(status) {}
  std::string serialize();
  std::expected<void, std::string> serialize(std::ostream &serialize_to);

  template <typename Self> auto &&status(this Self &&self) { return std::forward<Self>(self).status_internal; }
  template <typename Self> auto &&headers(this Self &&self) { return std::forward<Self>(self).headers_internal; }
  template <typename Self> auto &&body(this Self &&self) { return std::forward<Self>(self).body_internal; }
  
  private:
  HttpHeaders headers_internal;
  HttpBody body_internal;
  HttpStatus status_internal;
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

std::optional<std::string_view> http_status_to_reason(HttpStatus status);
HttpResponse get_redirection_response(std::string target);

enum class HttpStatus : unsigned short {
  Continue = 100,
  SwitchingProtocols = 101,
  Processing = 102,
  EarlyHints = 103,
  OK = 200,
  Created = 201,
  Accepted = 202,
  NonAuthoritativeInformation = 203,
  NoContent = 204,
  ResetContent = 205,
  PartialContent = 206,
  MultiStatus = 207,
  AlreadyReported = 208,
  IMUsed = 226,
  MultipleChoices = 300,
  MovedPermanently = 301,
  Found = 302,
  SeeOther = 303,
  NotModified = 304,
  UseProxy = 305,
  TemporaryRedirect = 307,
  PermanentRedirect = 308,
  BadRequest = 400,
  Unauthorized = 401,
  PaymentRequired = 402,
  Forbidden = 403,
  NotFound = 404,
  MethodNotAllowed = 405,
  NotAcceptable = 406,
  ProxyAuthenticationRequired = 407,
  RequestTimeout = 408,
  Conflict = 409,
  Gone = 410,
  LengthRequired = 411,
  PreconditionFailed = 412,
  PayloadTooLarge = 413,
  URITooLong = 414,
  UnsupportedMediaType = 415,
  RangeNotSatisfiable = 416,
  ExpectationFailed = 417,
  ImATeapot = 418,
  MisdirectedRequest = 421,
  UnprocessableEntity = 422,
  Locked = 423,
  FailedDependency = 424,
  TooEarly = 425,
  UpgradeRequired = 426,
  PreconditionRequired = 428,
  TooManyRequests = 429,
  RequestHeaderFieldsTooLarge = 431,
  UnavailableForLegalReasons = 451,
  InternalServerError = 500,
  NotImplemented = 501,
  BadGateway = 502,
  ServiceUnavailable = 503,
  GatewayTimeout = 504,
  HTTPVersionNotSupported = 505,
  VariantAlsoNegotiates = 506,
  InsufficientStorage = 507,
  LoopDetected = 508,
  NotExtended = 510,
  NetworkAuthenticationRequired = 511
};
}
#endif
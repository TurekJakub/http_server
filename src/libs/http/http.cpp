#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <istream>
#include <optional>
#include <print>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "../utils/fs_utils.h"
#include "../utils/string_utils.h"
#include "http.h"
#include "mime_type.h"

using namespace std;
using namespace http_server::strutils;
using namespace http_server::http;

namespace {
#define unwrap(result)                                                                                                                     \
  if (!result) {                                                                                                                           \
    return unexpected(result.error());                                                                                                     \
  }
} // namespace

namespace http_server::http {
expected<HttpResponse, std::string> HttpParser::parse_response(std::istream &input) {
  string header_line;
  getline(input, header_line);
  if (input.fail()) {
    return unexpected("Failed to parse http respones - first respones line missing");
  }

  auto tokens = split(header_line, " ");

  HttpStatus status_code = (HttpStatus)stoi(tokens[1]);
  if (!http_status_to_reason(status_code).has_value()) {
    return unexpected(format("Unknown HTTP status {}", (unsigned short)status_code));
  }

  auto parse_result = parse_headers(input);

  unwrap(parse_result)

      auto headers = parse_result.value();

  auto body_result = parse_body(input, headers);

  unwrap(body_result)

      return HttpResponse(headers, body_result.value(), status_code);
}

expected<HttpRequest, string> HttpParser::parse_request(istream &input) {
  string header_line;
  getline(input, header_line);
  if (input.fail()) {
    return unexpected("Failed to parse http respones - first respones line missing");
  }

  auto tokens = split(header_line, " ");
  if (tokens.size() != 3) {
    return unexpected("malformed request format, cannot determine method, requested url and http version");
  }

  HttpMethod method = str_to_method(tokens[0]);
  string requested_url = tokens[1];

  auto headers_parse_result = parse_headers(input);

  unwrap(headers_parse_result)

      HttpHeaders headers = headers_parse_result.value();

  auto parse_body_result = parse_body(input, headers);

  unwrap(parse_body_result)

      return HttpRequest{method, headers, parse_body_result.value(), requested_url};
}

expected<HttpHeaders, string> HttpParser::parse_headers(istream &input) {
  HttpHeaders headers;
  string line;
  while (getline(input, line)) {
    if (input.fail()) {
      return unexpected("failed to parse http response headers");
    }

    trim(line);
    if (line.size() == 0) {
      break;
    }

    auto tokens = split_n(line, ":", 1);
    for_each(tokens.begin(), tokens.end(), [](string &s) { return trim(s); });

    if (tokens.size() != 2) {
      return unexpected("not enough tokens in response header line");
    }

    headers.set(tokens[0], tokens[1]);
  }
  return headers;
}

expected<HttpBody, string> HttpParser::parse_chunked_body(istream &input) {
  string stream_line;
  HttpBody body;
  while (getline(input, stream_line)) {
    if (input.fail()) {
      return unexpected("failed to parse chunked http response body");
    }

    trim(stream_line);
    size_t chunk_size;
    if (from_chars(stream_line.data(), stream_line.data() + stream_line.size(), chunk_size, 16).ec != errc()) {
      return unexpected("filed to parse http respones - invalid chunk size");
    }

    if (chunk_size == 0) {
      return body;
    }

    size_t current_buff_size = body.size();
    body.resize(current_buff_size + chunk_size);

    if (!input.read(body.data() + current_buff_size, chunk_size)) {
      return unexpected("failed to parse http response chunk.");
    }

    input.ignore(2);
  }

  return unexpected("invalid response body format - chunked response must ends with empty chunk");
}

expected<HttpBody, string> HttpParser::parse_body(istream &input, const HttpHeaders &headers) {
  auto ct_length_header = headers.get("Content-Length");
  auto encoding_header = headers.get("Transfer-Encoding");

  if (ct_length_header) {
    string ctl_str = ct_length_header.value();
    size_t content_length = 0;

    if (from_chars(ctl_str.data(), ctl_str.data() + ctl_str.size(), content_length, 10).ec != errc()) {
      return unexpected("failed to parse http response - invalid content-length header, value: " + ctl_str);
    }

    HttpBody body(content_length);
    if (!input.read(body.data(), content_length)) {
      return unexpected("failed to parse http response body");
    }

    return body;
  } else if (encoding_header && encoding_header.value() == "chunked") {
    return parse_chunked_body(input);
  }
  return HttpBody(0);
}

void HttpHeaders::set(string header, string value) { headers.insert(make_pair(std::move(header), std::move(value))); }

void HttpHeaders::upsert(string header,string value) {headers.insert_or_assign(std::move(header), std::move(value));}

optional<string> HttpHeaders::get(string header) const {
  auto it = headers.find(header);
  if (it == headers.end()) {
    return nullopt;
  }
  return optional(it->second);
}

ranges::subrange<HttpHeader::const_iterator> HttpHeaders::get_all() { return ranges::subrange(headers.begin(), headers.end()); }

void HttpHeaders::clear(){
  headers.clear();
}

HttpMethod str_to_method(string method_str) {
  for (auto &&method : method_map) {
    if (method.first == method_str) {
      return method.second;
    }
  }
  return method_str;
}

bool operator==(const HttpMethod& method, const std::string& str) {
  auto* val = std::get_if<std::string>(&method);
  if (val) {
    return *val == str;
  }
  return false;
}

optional<string_view> http_status_to_reason(HttpStatus status) {
  using status_reason_mapping = pair<HttpStatus, string_view>;
  static constexpr array<status_reason_mapping, 62> mapping = {
      {{HttpStatus::Continue, "Continue"},
       {HttpStatus::SwitchingProtocols, "Switching Protocols"},
       {HttpStatus::Processing, "Processing"},
       {HttpStatus::EarlyHints, "Early Hints"},

       {HttpStatus::OK, "OK"},
       {HttpStatus::Created, "Created"},
       {HttpStatus::Accepted, "Accepted"},
       {HttpStatus::NonAuthoritativeInformation, "Non-Authoritative Information"},
       {HttpStatus::NoContent, "No Content"},
       {HttpStatus::ResetContent, "Reset Content"},
       {HttpStatus::PartialContent, "Partial Content"},
       {HttpStatus::MultiStatus, "Multi-Status"},
       {HttpStatus::AlreadyReported, "Already Reported"},
       {HttpStatus::IMUsed, "IM Used"},

       {HttpStatus::MultipleChoices, "Multiple Choices"},
       {HttpStatus::MovedPermanently, "Moved Permanently"},
       {HttpStatus::Found, "Found"},
       {HttpStatus::SeeOther, "See Other"},
       {HttpStatus::NotModified, "Not Modified"},
       {HttpStatus::UseProxy, "Use Proxy"},
       {HttpStatus::TemporaryRedirect, "Temporary Redirect"},
       {HttpStatus::PermanentRedirect, "Permanent Redirect"},

       {HttpStatus::BadRequest, "Bad Request"},
       {HttpStatus::Unauthorized, "Unauthorized"},
       {HttpStatus::PaymentRequired, "Payment Required"},
       {HttpStatus::Forbidden, "Forbidden"},
       {HttpStatus::NotFound, "Not Found"},
       {HttpStatus::MethodNotAllowed, "Method Not Allowed"},
       {HttpStatus::NotAcceptable, "Not Acceptable"},
       {HttpStatus::ProxyAuthenticationRequired, "Proxy Authentication Required"},
       {HttpStatus::RequestTimeout, "Request Timeout"},
       {HttpStatus::Conflict, "Conflict"},
       {HttpStatus::Gone, "Gone"},
       {HttpStatus::LengthRequired, "Length Required"},
       {HttpStatus::PreconditionFailed, "Precondition Failed"},
       {HttpStatus::PayloadTooLarge, "Payload Too Large"},
       {HttpStatus::URITooLong, "URI Too Long"},
       {HttpStatus::UnsupportedMediaType, "Unsupported Media Type"},
       {HttpStatus::RangeNotSatisfiable, "Range Not Satisfiable"},
       {HttpStatus::ExpectationFailed, "Expectation Failed"},
       {HttpStatus::ImATeapot, "I'm a teapot"},
       {HttpStatus::MisdirectedRequest, "Misdirected Request"},
       {HttpStatus::UnprocessableEntity, "Unprocessable Entity"},
       {HttpStatus::Locked, "Locked"},
       {HttpStatus::FailedDependency, "Failed Dependency"},
       {HttpStatus::TooEarly, "Too Early"},
       {HttpStatus::UpgradeRequired, "Upgrade Required"},
       {HttpStatus::PreconditionRequired, "Precondition Required"},
       {HttpStatus::TooManyRequests, "Too Many Requests"},
       {HttpStatus::RequestHeaderFieldsTooLarge, "Request Header Fields Too Large"},
       {HttpStatus::UnavailableForLegalReasons, "Unavailable For Legal Reasons"},

       {HttpStatus::InternalServerError, "Internal Server Error"},
       {HttpStatus::NotImplemented, "Not Implemented"},
       {HttpStatus::BadGateway, "Bad Gateway"},
       {HttpStatus::ServiceUnavailable, "Service Unavailable"},
       {HttpStatus::GatewayTimeout, "Gateway Timeout"},
       {HttpStatus::HTTPVersionNotSupported, "HTTP Version Not Supported"},
       {HttpStatus::VariantAlsoNegotiates, "Variant Also Negotiates"},
       {HttpStatus::InsufficientStorage, "Insufficient Storage"},
       {HttpStatus::LoopDetected, "Loop Detected"},
       {HttpStatus::NotExtended, "Not Extended"},
       {HttpStatus::NetworkAuthenticationRequired, "Network Authentication Required"}}
  };

  auto res = lower_bound(mapping.begin(), mapping.end(), status, [](const status_reason_mapping &m, HttpStatus s) {
    auto [status_code, reason] = m;
    return status_code < s;
  });

  if (res != mapping.end() && res->first == status) {
    return res->second;
  }
  return nullopt;
}

string HttpResponse::serialize() {
  stringstream buffer;
  auto result = serialize(buffer);
  if (!result.has_value()) {
    // Just return empty string in case of error, cause stringstream is unlikely to fail
    return "";
  }
  return buffer.str();
}

expected<void, string> HttpResponse::serialize(ostream &serialize_to) {
  headers_internal.upsert("Content-Length", to_string(body_internal.size()));
  string http_reason = string(http_status_to_reason(status_internal).value_or(""));
  print(serialize_to, "HTTP/1.1 {} {}\r\n", (unsigned int)status_internal, http_reason);
  if (serialize_to.fail()) {
    return unexpected("failed to serialize respones");
  }
  for (auto &&header : headers_internal.get_all()) {
    print(serialize_to, "{}: {}\r\n", header.first, header.second);
    if (serialize_to.fail()) {
      return unexpected("failed to serialize respones headers");
    }
  }
  print(serialize_to, "\r\n");
  if (serialize_to.fail()) {
    return unexpected("failed to serialize response body");
  }
  serialize_to.write(body_internal.data(), body_internal.size());
  if (serialize_to.fail()) {
    return unexpected("failed to serialize response body");
  }
  return {};
}

HttpResponse get_redirection_response(string target) {
  return {{{{"Location", target}, {"Content-Length", "0"}, {"Connection", "close"}}}, {}, HttpStatus::MovedPermanently};
}

expected<void, string> serve_file(const string &file_path, HttpResponse &res) {
  std::filesystem::path file(file_path);
  res.status() = HttpStatus::OK;
  string mime(mimetypes::get_mime_for_file(file).value_or("application/octet-stream"));
  res.headers().set("Content-Type", mime);
  res.headers().set("Connection", "keep-alive");
  res.headers().set("X-Content-Type-Options", "nosniff");

  auto read_res = http_server::fsutils::read_file_to_buffer(file, res.body());
  if (!read_res.has_value()) {
    return unexpected(read_res.error());
  }

  return {};
}

} // namespace http_server::http
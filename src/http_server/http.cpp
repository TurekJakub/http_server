#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <expected>
#include <istream>
#include <optional>
#include <ranges>
#include <string>
#include <system_error>
#include <utility>

#include "http.h"
#include "string_utils.h"

using namespace std;
using namespace strutils;

expected<HttpResponse, std::string> HttpParser::parse_response(std::istream &input) {
  string header_line;
  getline(input, header_line);
  if (input.fail()) {
    return unexpected("Failed to parese http respones - first respones line missing");
  }

  auto tokens = split(header_line, " ");
  unsigned int status_code = stoi(tokens[1]);

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
    return unexpected("Failed to parese http respones - first respones line missing");
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

void HttpHeaders::set(string header, string value) { headers.insert(make_pair(header, value)); }

ranges::subrange<HttpHeader::const_iterator> HttpHeaders::get_all() { return ranges::subrange(headers.begin(), headers.end()); }

optional<string> HttpHeaders::get(string header) const {
  auto it = headers.find(header);
  if (it == headers.end()) {
    return nullopt;
  }
  return optional(it->second);
}

HttpMethod str_to_method(string method_str) {
  for (auto &&method : method_map) {
    if (method.first == method_str) {
      return method.second;
    }
  }
  return method_str;
}
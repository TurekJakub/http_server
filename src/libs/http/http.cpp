#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <expected>
#include <istream>
#include <optional>
#include <print>
#include <ranges>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>

#include "http.h"
#include "../utils/string_utils.h"

using namespace std;
using namespace strutils;

expected<HttpResponse, std::string> HttpParser::parse_response(std::istream &input) {
  string header_line;
  getline(input, header_line);
  if (input.fail()) {
    return unexpected("Failed to parse http respones - first respones line missing");
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

optional<string> http_status_to_reason(unsigned short status_code) {
  static const unordered_map<unsigned short, string> mapping = {
      {100, "Continue"                       },
      {101, "Switching Protocols"            },
      {102, "Processing"                     },
      {103, "Early Hints"                    },

      {200, "OK"                             },
      {201, "Created"                        },
      {202, "Accepted"                       },
      {203, "Non-Authoritative Information"  },
      {204, "No Content"                     },
      {205, "Reset Content"                  },
      {206, "Partial Content"                },
      {207, "Multi-Status"                   },
      {208, "Already Reported"               },
      {226, "IM Used"                        },

      {300, "Multiple Choices"               },
      {301, "Moved Permanently"              },
      {302, "Found"                          },
      {303, "See Other"                      },
      {304, "Not Modified"                   },
      {305, "Use Proxy"                      },
      {307, "Temporary Redirect"             },
      {308, "Permanent Redirect"             },

      {400, "Bad Request"                    },
      {401, "Unauthorized"                   },
      {402, "Payment Required"               },
      {403, "Forbidden"                      },
      {404, "Not Found"                      },
      {405, "Method Not Allowed"             },
      {406, "Not Acceptable"                 },
      {407, "Proxy Authentication Required"  },
      {408, "Request Timeout"                },
      {409, "Conflict"                       },
      {410, "Gone"                           },
      {411, "Length Required"                },
      {412, "Precondition Failed"            },
      {413, "Payload Too Large"              },
      {414, "URI Too Long"                   },
      {415, "Unsupported Media Type"         },
      {416, "Range Not Satisfiable"          },
      {417, "Expectation Failed"             },
      {418, "I'm a teapot"                   },
      {421, "Misdirected Request"            },
      {422, "Unprocessable Entity"           },
      {423, "Locked"                         },
      {424, "Failed Dependency"              },
      {425, "Too Early"                      },
      {426, "Upgrade Required"               },
      {428, "Precondition Required"          },
      {429, "Too Many Requests"              },
      {431, "Request Header Fields Too Large"},
      {451, "Unavailable For Legal Reasons"  },

      {500, "Internal Server Error"          },
      {501, "Not Implemented"                },
      {502, "Bad Gateway"                    },
      {503, "Service Unavailable"            },
      {504, "Gateway Timeout"                },
      {505, "HTTP Version Not Supported"     },
      {506, "Variant Also Negotiates"        },
      {507, "Insufficient Storage"           },
      {508, "Loop Detected"                  },
      {510, "Not Extended"                   },
      {511, "Network Authentication Required"}
  };

  auto res = mapping.find(status_code);
  if (res != mapping.end()) {
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
  string http_reason = http_status_to_reason(status).value_or("");
  print(serialize_to, "HTTP/1.1 {} {}\r\n", status, http_reason);
  if (serialize_to.fail()) {
    return unexpected("failed to serialize respones");
  }
  for (auto &&header : headers.get_all()) {
    print(serialize_to, "{}: {}\r\n", header.first, header.second);
    if (serialize_to.fail()) {
      return unexpected("failed to serialize respones headers");
    }
  }
  print(serialize_to, "\r\n");
  if (serialize_to.fail()) {
    return unexpected("failed to serialize response body");
  }
  serialize_to.write(body.data(), body.size());
  if (serialize_to.fail()) {
    return unexpected("failed to serialize response body");
  }
  return {};
}

HttpResponse get_redirection_response(string target) {
  return {{{{"Location", target}, {"Content-Length", "0"}, {"Connection", "close"}}}, {}, 301};
}
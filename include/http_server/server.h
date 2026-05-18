#ifndef HTTP_SERVER_H

#define HTTP_SERVER_H

#include <concepts>
#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "http.h"

namespace http_server::http {
class HandlerError {
public:
  HandlerError(std::string message, HttpStatus status = HttpStatus::InternalServerError)
      : status_internal(status), message_internal(std::move(message)) {};
  inline const HttpStatus &status() { return status_internal; }
  inline const std::string &message() { return message_internal; }

private:
  HttpStatus status_internal;
  std::string message_internal;
};

template <typename T>
concept handler = requires(T handler_func, const HttpRequest &req, HttpResponse &resp) {
  { handler_func(req, resp) } -> std::same_as<void>;
} || requires(T handler_func, const HttpRequest &req, HttpResponse &resp) {
  { handler_func(req, resp) } -> std::same_as<std::expected<void, HandlerError>>;
};

using handler_function = std::move_only_function<std::expected<void, HandlerError>(const HttpRequest &, HttpResponse &)>;

class HttpServerConfig {
public:
  unsigned short port = {443};
  std::string cert_path = {""};
  std::string private_key_path = {""};
  unsigned int max_thread_count = {1};
};

class HttpServer {
public:
  HttpServer(HttpServerConfig config);
  ~HttpServer();

  void start();
  template <handler T> void add_handler(std::string route, T &&handler_func, bool prefix_match = false) {
    do_add_handler(std::move(route), wrap_handler(std::forward<T>(handler_func)), prefix_match);
  };
  template <handler T> void set_default_handler(T &&default_handler) {
    do_set_default_handler(wrap_handler(std::forward<T>(default_handler)));
  }

private:
  template <handler T> handler_function wrap_handler(T &&handler_func) {
    return
        [handler = std::forward<T>(handler_func)](const HttpRequest &req, HttpResponse &res) mutable -> std::expected<void, HandlerError> {
          using return_type = std::invoke_result_t<std::decay_t<T>, const HttpRequest &, HttpResponse &>;
          if constexpr (std::is_same_v<return_type, void>) {
            handler(req, res);
            return {};
          } else {
            return handler(req, res);
          }
        };
  }

  void do_add_handler(std::string route, handler_function handler, bool prefix_match);
  void do_set_default_handler(handler_function handler);

  class Impl;

  std::unique_ptr<Impl> impl;
};
} // namespace http
#endif
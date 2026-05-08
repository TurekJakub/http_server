#ifndef HTTP_SERVER_H

#define HTTP_SERVER_H

#include "asio/ip/tcp.hpp"
#include "asio/ssl/context.hpp"
#include "asio/ssl/stream.hpp"
#include "asio/strand.hpp"
#include "http.h"
#include <algorithm>
#include <asio.hpp>
#include <concepts>
#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "../../http_server/config_parser.h"

class HandlerError {
public:
  HandlerError(std::string message, unsigned short status = 500) : status(status), message(std::move(message)) {};
  unsigned short status;
  std::string message;
};
template <typename T>
concept handler = requires(T handler_func, const HttpRequest &req, HttpResponse &resp) {
  { handler_func(req, resp) } -> std::same_as<void>;
} || requires(T handler_func, const HttpRequest &req, HttpResponse &resp) {
  { handler_func(req, resp) } -> std::same_as<std::expected<void, HandlerError>>;
};
class Router {
public:
  using handler_function = std::move_only_function<std::expected<void, HandlerError>(const HttpRequest &, HttpResponse &)>;
  Router();
  template <handler T> Router(T &&default_handler) : routing_table(), default_handler(default_handler(std::forward<T>(default_handler))) {};
  void route(HttpRequest &req, HttpResponse &resp);
  template <handler T> void add_handler(std::string route, T &&handler_func, bool prefix_match = false) {
    routing_table.insert_or_assign(std::move(route), std::make_pair(wrap_handler(std::forward<T>(handler_func)), prefix_match));
  }
  template <handler T> void set_default_handler(T &&handler) { default_handler = wrap_handler(std::forward<T>(handler)); }

private:
  using routing_table_record = std::pair<handler_function, bool>;
  using route_table = std::map<std::string, routing_table_record>;

  template <handler T> handler_function wrap_handler(T &&handler_func) {
    return [handler = std::forward<T>(handler_func)](const HttpRequest &req, HttpResponse &res) mutable -> std::expected<void, HandlerError> {
      using return_type = std::invoke_result_t<std::decay_t<T>, const HttpRequest &, HttpResponse &>;
      if constexpr (std::is_same_v<return_type, void>) {
        handler(req, res);
        return {};
      } else {
        return handler(req, res);
      }
    };
  }

  void invoke_handler(handler_function &handler, const HttpRequest &req, HttpResponse &res);

  route_table routing_table;
  handler_function default_handler;
};
class Connection : public std::enable_shared_from_this<Connection> {
public:
  typedef asio::ssl::stream<asio::ip::tcp::socket> ssl_socket;
  Connection(ssl_socket socket, Router &router, asio::io_context &io_context)
      : socket(std::move(socket)), router(router), strand_executor(asio::make_strand(io_context)) {};
  void start();

private:
  void read();
  void handshake();
  void write(std::string message, bool keepAlive);
  void redirect_to_https();

  ssl_socket socket;
  asio::ip::tcp::endpoint endpoint;
  asio::streambuf buffer;
  Router &router;
  asio::strand<asio::io_context::executor_type> strand_executor;
};

class HttpServer {
public:
  HttpServer(asio::io_context &io_context, ServerConfig config);
  void start();
  template <handler T> void add_handler(std::string route, T &&handler_func, bool prefix_match = false) {
    router.add_handler(std::move(route), std::forward<T>(handler_func), prefix_match);
  };
  template <handler T> void set_default_handler(T &&default_handler) { router.set_default_handler(std::move(default_handler)); }

private:
  void accept_connection();

  asio::io_context &context;
  asio::ip::tcp::acceptor acceptor;
  asio::ssl::context ssl_context;
  unsigned short port;
  Router router;
};

#endif
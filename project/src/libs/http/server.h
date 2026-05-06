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
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "../../http_server/config_parser.h"

template <typename T>
concept handler = requires(T handler_func, const HttpRequest &req, HttpResponse &resp) {
  { handler_func(req, resp) } -> std::same_as<void>;
};
class Router {
public:
  using handler_function = std::move_only_function<void(const HttpRequest &, HttpResponse &)>;
  Router();
  template <handler T> Router(T &&default_handler) : routing_table(), default_handler(default_handler(std::forward<T>(default_handler))) {};
  void route(const HttpRequest &req, HttpResponse &resp);
  template <handler T> void add_handler(std::string route, T &&handler_func) {
    routing_table.insert_or_assign(std::move(route), handler_function(std::forward<T>(handler_func)));
  }
  template <handler T> void set_default_handler(T &&handler) { default_handler = handler_function(std::forward<T>(handler)); }

private:
  std::map<std::string, handler_function> routing_table;
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
  template <handler T> void add_handler(std::string route, T &&handler_func) {
    router.add_handler(std::move(route), std::forward<T>(handler_func));
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
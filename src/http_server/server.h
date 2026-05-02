#ifndef HTTP_SERVER_H

#define HTTP_SERVER_H

#include "asio/ip/tcp.hpp"
#include "asio/ssl/context.hpp"
#include "asio/ssl/stream.hpp"
#include "asio/strand.hpp"
#include "http.h"
#include <asio.hpp>
#include <memory>
#include <string>

class Router {
public:
  void handle(const HttpRequest &req, HttpResponse &resp);

private:
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

struct ServerConfig {
public:
  unsigned short port;
  std::string cert_path;
  std::string private_key_path;
};
class HttpServer {
public:
  HttpServer(asio::io_context &io_context, ServerConfig config, Router router);
  void start();

private:
  void accept_connection();

  asio::io_context &context;
  asio::ip::tcp::acceptor acceptor;
  asio::ssl::context ssl_context;
  unsigned short port;
  Router router;
};

#endif
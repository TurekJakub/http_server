#ifndef HTTP_SERVER_H

#define HTTP_SERVER_H

#include "asio/ip/tcp.hpp"
#include "http.h"
#include <asio.hpp>
#include <memory>

class Router {
public:
  void handle(const HttpRequest &req, HttpResponse &resp);

private:
};

class Connection : public std::enable_shared_from_this<Connection> {
public:
  Connection(asio::ip::tcp::socket socket, Router &router);
  void start();

private:
  void read();
  void write(std::string message, bool keepAlive);

  asio::ip::tcp::socket socket;
  asio::ip::tcp::endpoint endpoint;
  asio::streambuf buffer;
  Router &router;
};

class HttpServer {
public:
  HttpServer(asio::io_context &io_context, unsigned short port, Router router)
      : acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), port(port), router(router) {};
  void start();

private:
  void accept_connection();

  asio::ip::tcp::acceptor acceptor;
  unsigned short port;
  Router router;
};

#endif
#include "server.h"
#include "asio/ip/tcp.hpp"
#include "asio/socket_base.hpp"
#include "http.h"
#include <asio.hpp>
#include <cstdio>
#include <format>
#include <iostream>
#include <memory>
#include <print>
#include <string>

using namespace asio::ip;
using namespace asio;
using namespace std;

Connection::Connection(tcp::socket socket, Router &router) : socket(std::move(socket)), router(router) {
 // endpoint = socket.remote_endpoint();
}

void Connection::read() {
  auto self(shared_from_this());
  auto dest = buffer.prepare(8192);

  socket.async_read_some(dest, [this, self](const error_code &ec, size_t n) {
    if (ec) {
      if (ec != error::eof) {
        cerr << format("Recieving data failed, err: {}", ec.message());
      }
      return;
    }

    buffer.commit(n);

    std::istream input_stream(&buffer);
    HttpParser parser;

    auto parse_result = parser.parse_request(input_stream);
    if (!parse_result) {
      cerr << format("Parsing incoming request fails, err: {}", parse_result.error());
      return;
    }

    auto req = parse_result.value();

    HttpResponse res;
    router.handle(req, res);

    auto connection_header = req.headers.get("Connection");
    bool keepAlive = connection_header && connection_header.value() == "Keep-Alive";

    write(res.serialize(), keepAlive);
  });
}

void Connection::start() {
  try {
    // **Safely get the endpoint here**
    endpoint = socket.remote_endpoint();
    read();
  } catch (const asio::system_error &e) {
  }
}

void Connection::write(string message, bool keepAlive) {
  auto self(shared_from_this());

  asio::async_write(socket, asio::buffer(message), [this, keepAlive, self](const asio::error_code &ec, std::size_t /*length*/) {
    if (ec) {
      cerr << format("Writing data failed, error: {}", ec.message());
      return;
    }

    if (keepAlive) {
      read();
    } else {
      socket.shutdown(asio::socket_base::shutdown_send);
    }
  });
}

void HttpServer::start() {
  println("Server listening on port {}", port);
  accept_connection();
}

void HttpServer::accept_connection() {
  acceptor.async_accept([this](const asio::error_code &ec, asio::ip::tcp::socket socket) {
    if (ec) {
      cerr << format("Failed to estabilish connection, error: {}", ec.message());

    } else {

      println("New connection accepted");
      make_shared<Connection>(std::move(socket), router)->start();
    }

    accept_connection();
  });
}

void Router::handle(const HttpRequest &req, HttpResponse &resp) {
  auto _ = req;
  auto __ = resp;
  return;
}

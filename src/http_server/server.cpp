#include "server.h"
#include "asio/ip/tcp.hpp"
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

void Connection::start() {
  try {
    endpoint = socket.lowest_layer().remote_endpoint();
  } catch (const asio::system_error &e) {
    cerr << format("Failed to establish connection - failed to obtain socket remote endpoint err: {}", e.what());
    return;
  }

  handshake();
}

void ::Connection::handshake() {
  auto self = shared_from_this();
  socket.async_handshake(asio::ssl::stream_base::server, [self](const asio::error_code &ec) {
    if (ec) {
      std::cerr << format("TLS handshake failed with error: {}\n", ec.message());
      return;
    }
    self->read();
  });
}

void Connection::read() {
  auto self(shared_from_this());
  auto dest = buffer.prepare(8192);

  socket.async_read_some(dest, [this, self](const error_code &ec, size_t n) {
    if (ec) {
      if (ec != error::eof) {
        cerr << format("Receiving data failed, err: {}", ec.message());
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

void Connection::write(string message, bool keepAlive) {
  auto self(shared_from_this());

  async_write(socket, asio::buffer(message), [this, keepAlive, self](const error_code &ec, std::size_t /*length*/) {
    if (ec) {
      cerr << format("Writing data failed, error: {}", ec.message());
      return;
    }

    if (keepAlive) {
      read();
    } else {
      socket.shutdown();
    }
  });
}

HttpServer::HttpServer(io_context &io_context, ServerConfig config, Router router)
: acceptor(io_context, tcp::endpoint(tcp::v4(), config.port)), ssl_context(ssl::context::tlsv13_server),
port(config.port), router(router) {
  try {
    using namespace asio::ssl;
    ssl_context.set_password_callback([](size_t, context_base::password_purpose) {
      const char *pw = getenv("PRIVATE_KEY_PASS");
      if (pw) {
        return pw;
      }
      return "";
    });

    ssl_context.use_certificate_file(config.cert_path, context::pem);
    ssl_context.use_private_key_file(config.private_key_path, context::pem);
  } catch (const exception &err) {
    cerr << format("Error occurred during TLS configuration, err: {}\n", err.what());
  }
};

void HttpServer::start() {
  println("Server listening on port {}", port);
  accept_connection();
}

void HttpServer::accept_connection() {
  acceptor.async_accept([this](const asio::error_code &ec, asio::ip::tcp::socket socket) {
    if (ec) {
      cerr << format("Failed to establish connection, error: {}", ec.message());

    } else {

      println("New connection accepted");
      make_shared<Connection>(Connection::ssl_socket(std::move(socket), ssl_context), router)->start();
    }

    accept_connection();
  });
}

void Router::handle(const HttpRequest &req, HttpResponse &resp) {
  auto _ = req;
  auto __ = resp;
  return;
}

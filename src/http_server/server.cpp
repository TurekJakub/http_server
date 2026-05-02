#include "server.h"
#include "asio/bind_executor.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/streambuf.hpp"
#include "asio/write.hpp"
#include "http.h"
#include <asio.hpp>
#include <cstdio>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <string>

using namespace asio::ip;
using namespace asio;
using namespace std;

void Connection::start() {
  try {
    endpoint = socket.lowest_layer().remote_endpoint();
  } catch (const asio::system_error &e) {
    print(cerr, "Failed to establish connection - failed to obtain socket remote endpoint err: {}", e.what());
    return;
  }

  auto self = shared_from_this();

  async_read(socket.next_layer(), buffer, asio::transfer_exactly(1), bind_executor(strand_executor, [this, self](const error_code &ec, size_t) {
    if (ec) {
      println(cerr, "Failed to establish new connection, err: {}", ec.message());
      return;
    }

    const unsigned char *data = static_cast<const unsigned char *>(buffer.data().data());

    if (data[0] == TLS_HANDSHAKE_IDENTIFIER_BYTE) {
      handshake();
    } else {
      redirect_to_https();
    }
  }));
}

void ::Connection::handshake() {
  auto self = shared_from_this();
  socket.async_handshake(asio::ssl::stream_base::server, buffer.data(), bind_executor(strand_executor,[self](const asio::error_code &ec, size_t size) {
    if (ec) {
      print(cerr, "TLS handshake failed with error: {}\n", ec.message());
      return;
    }

    self->buffer.consume(size);

    self->read();
  }));
}

void Connection::read() {
  auto self(shared_from_this());
  auto dest = buffer.prepare(8192);

  socket.async_read_some(dest, bind_executor(strand_executor,[this, self](const error_code &ec, size_t n) {
    if (ec) {
      if (ec != error::eof) {
        print(cerr, "Receiving data failed, err: {}", ec.message());
      }
      return;
    }

    buffer.commit(n);

    std::istream input_stream(&buffer);
    HttpParser parser;

    auto parse_result = parser.parse_request(input_stream);
    if (!parse_result) {
      print(cerr, "Parsing incoming request fails, err: {}", parse_result.error());
      return;
    }

    auto req = parse_result.value();

    HttpResponse res;
    router.handle(req, res);

    auto connection_header = req.headers.get("Connection");
    bool keepAlive = connection_header && connection_header.value() == "Keep-Alive";

    write(res.serialize(), keepAlive);
  }));
}

void Connection::write(string message, bool keepAlive) {
  auto self(shared_from_this());

  async_write(socket, asio::buffer(message), bind_executor(strand_executor,[this, keepAlive, self](const error_code &ec, std::size_t) {
    if (ec) {
      print(cerr, "Writing data failed, error: {}", ec.message());
      return;
    }

    if (keepAlive) {
      read();
    } else {
      socket.shutdown();
    }
  }));
}

void Connection::redirect_to_https() {
  auto self = shared_from_this();
  asio::async_read_until(socket.next_layer(), buffer, HTTP_BODY_DELIMITER, bind_executor(strand_executor,[this, self](const error_code &ec, size_t) {
    if (ec) {
      if (ec != error::eof) {
        println(cerr, "Receiving data failed, err: {}", ec.message());
      }
      return;
    }

    std::istream input_stream(&buffer);
    HttpParser parser;

    auto parse_result = parser.parse_request(input_stream);
    if (!parse_result) {
      println(cerr, "Parsing incoming request fails, err: {}", parse_result.error());
      return;
    }

    auto req = parse_result.value();

    optional<string> host_opt = req.headers.get("Host");
    if (!host_opt.has_value()) {
      println(cerr, "Failed to determine host while redirecting request to HTTPS");
      return;
    }
    string host = host_opt.value();

    HttpResponse redirect = get_redirection_response(format("https://{}{}", host, req.requested_url));

    auto write_buffer = std::make_shared<asio::streambuf>();
    std::ostream output_stream(write_buffer.get());

    auto serialization_result  = redirect.serialize(output_stream);
    if (!serialization_result.has_value()){
        println(cerr, "Failed to serialize HTTPS redirection response");
    }

    async_write(socket.next_layer(), *write_buffer, bind_executor(strand_executor,[this, self, write_buffer](const error_code &ec, std::size_t) {
      if (ec) {
        println(cerr, "Failed to send redirect to HTTPS, err: {}", ec.message());
      }
      socket.next_layer().close();
    }));
  }));
}

HttpServer::HttpServer(io_context &io_context, ServerConfig config, Router router)
    : context(io_context), acceptor(io_context, tcp::endpoint(tcp::v4(), config.port)), ssl_context(ssl::context::tlsv13_server), port(config.port),
      router(router) {
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
    print(cerr, "Error occurred during TLS configuration, err: {}\n", err.what());
  }
};

void HttpServer::start() {
  println("Server listening on port {}", port);
  accept_connection();
}

void HttpServer::accept_connection() {
  acceptor.async_accept([this](const asio::error_code &ec, asio::ip::tcp::socket socket) {
    if (ec) {
      print(cerr, "Failed to establish connection, error: {}", ec.message());

    } else {

      println("New connection accepted");
      make_shared<Connection>(Connection::ssl_socket(std::move(socket), ssl_context), router, context)->start();
    }

    accept_connection();
  });
}

void Router::handle(const HttpRequest &req, HttpResponse &resp) {
  auto _ = req;
  resp.status = 200;
  resp.headers.set("Content-Type", "text/html");
  resp.headers.set("Content-Length", "64");
  string body_val = "<!doctype html><html><body><h1>Hello, World !</h1></body></html>";
  resp.body = HttpBody{body_val.begin(), body_val.end()};
  return;
}

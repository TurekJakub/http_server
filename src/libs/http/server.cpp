#include "server.h"
#include "asio/bind_executor.hpp"
#include "asio/error_code.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/ssl/stream.hpp"
#include "asio/streambuf.hpp"
#include "asio/system_error.hpp"
#include "asio/write.hpp"
#include "http.h"
#include <asio.hpp>
#include <cstdio>
#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <string>

using namespace asio::ip;
using namespace asio;
using namespace std;
using namespace http_server::http;

namespace {
 constexpr unsigned char TLS_HANDSHAKE_IDENTIFIER_BYTE = 0x16;
 constexpr std::string HTTP_BODY_DELIMITER = "\r\n\r\n";
class Router {
public:
  Router();
  Router(handler_function default_handler) : routing_table(), default_handler(std::move(default_handler)) {};
  void route(HttpRequest &req, HttpResponse &resp);
  void add_handler(string route, handler_function handler_func, bool prefix_match = false);
  void set_default_handler(handler_function handler);

private:
  using routing_table_record = pair<handler_function, bool>;
  using route_table = map<string, routing_table_record>;

  void invoke_handler(handler_function &handler, const HttpRequest &req, HttpResponse &res);

  route_table routing_table;
  handler_function default_handler;
};

class Connection : public enable_shared_from_this<Connection> {
public:
  using ssl_socket = ssl::stream<tcp::socket>;
  Connection(ssl_socket socket, Router &router, io_context &io_context)
      : socket(std::move(socket)), router(router), strand_executor(make_strand(io_context)) {};
  void start();

private:
  void read();
  void handshake();
  void write(shared_ptr<string> message, bool keepAlive);
  void redirect_to_https();

  ssl_socket socket;
  tcp::endpoint endpoint;
  asio::streambuf buffer;
  Router &router;
  strand<io_context::executor_type> strand_executor;
};
} // namespace

void Connection::start() {
  try {
    endpoint = socket.lowest_layer().remote_endpoint();
  } catch (const asio::system_error &e) {
    print(cerr, "Failed to establish connection - failed to obtain socket remote endpoint err: {}", e.what());
    return;
  }

  auto self = shared_from_this();

  // clang-format off
  async_read(socket.next_layer(), buffer, asio::transfer_exactly(1),bind_executor(strand_executor, [this, self](const error_code &ec, size_t) {
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
// clang-format on

// clang-format off
void ::Connection::handshake() {
  auto self = shared_from_this();
  socket.async_handshake(asio::ssl::stream_base::server, buffer.data(),bind_executor(strand_executor, [self](const asio::error_code &ec, size_t size) {
  if (ec) {
    print(cerr, "TLS handshake failed with error: {}\n", ec.message());
    return;
  }
  self->buffer.consume(size);
  self->read();
}));
}
// clang-format on

// clang-format off
void Connection::read() {
  auto self(shared_from_this());
  auto dest = buffer.prepare(8192);

  socket.async_read_some(dest, bind_executor(strand_executor, [this, self](const error_code &ec, size_t n) {
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

  router.route(req, res);

  auto connection_header = req.headers.get("Connection");
  bool keepAlive = connection_header && connection_header.value() == "Keep-Alive";

  write(make_shared<string>(res.serialize()), keepAlive);
  }));
}
// clang-format on

// clang-format off
void Connection::write(shared_ptr<string> message, bool keepAlive) {
  auto self(shared_from_this());

  async_write(socket, asio::buffer(*message),bind_executor(strand_executor, [this, keepAlive, self, message](const error_code &ec, std::size_t) {
  if (ec) {
    print(cerr, "Writing data failed, error: {}", ec.message());
    return;
  }
  if (keepAlive) {
    read();
  } else {
    asio::error_code ec;
    if (socket.shutdown(ec)) {
      // This errors should be generally safe to ignore, but log them for debugging purposes
      println(cerr, "Connection shutdown error: {}", ec.message());
    }
  }
  }));
}
// clang-format on

// clang-format off
void Connection::redirect_to_https() {
  auto self = shared_from_this();
  asio::async_read_until(socket.next_layer(), buffer, HTTP_BODY_DELIMITER,bind_executor(strand_executor, [this, self](const error_code &ec, size_t) {
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

  auto serialization_result = redirect.serialize(output_stream);
  if (!serialization_result.has_value()) {
    println(cerr, "Failed to serialize HTTPS redirection response");
  }

  async_write(socket.next_layer(), *write_buffer,bind_executor(strand_executor, [this, self, write_buffer](const error_code &ec, std::size_t) {
    if (ec) {
      println(cerr, "Failed to send redirect to HTTPS, err: {}", ec.message());
    }
    socket.next_layer().close();
  }));

  }));
}
// clang-format on

namespace http_server::http {
  class HttpServer::Impl {
    public:
    Impl(HttpServerConfig config)
      : router(), context(config.max_thread_count), acceptor(context, tcp::endpoint(tcp::v4(), config.port)),
      ssl_context(ssl::context::tlsv13_server), port(config.port), max_concurency(config.max_thread_count) {
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
  }
  
  void start();
  
  Router router;
  
  private:
  void accept_connection();
  
  asio::io_context context;
  std::vector<std::thread> thread_pool;
  asio::ip::tcp::acceptor acceptor;
  asio::ssl::context ssl_context;
  unsigned short port;
  unsigned int max_concurency;
};

HttpServer::HttpServer(HttpServerConfig config) : impl(make_unique<Impl>(config)) {};

HttpServer::~HttpServer() = default;

void HttpServer::Impl::start() {
  println("Server listening on port {}", port);
  accept_connection();
  
  const unsigned int thread_count = max(1u, max_concurency);
  thread_pool.reserve(thread_count);
  
  for (size_t i = 0; i < thread_count; ++i) {
    thread_pool.emplace_back([this]() { this->context.run(); });
  }
  
  for (auto &t : thread_pool) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void HttpServer::Impl::accept_connection() {
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

void HttpServer::start() { impl->start(); }

void HttpServer::do_add_handler(string route, handler_function handler, bool prefix_match) {
  impl->router.add_handler(std::move(route), std::move(handler), prefix_match);
}

void HttpServer::do_set_default_handler(handler_function handler) { impl->router.set_default_handler(std::move(handler)); }
}

// clang-format off
Router::Router(): routing_table(), default_handler([](const HttpRequest &req, HttpResponse &resp) -> std::expected<void, HandlerError>{
  auto &_ = req;
  resp.status() = HttpStatus::OK;
  resp.headers().set("Content-Type", "text/html");
  string body_content = "<html><body><h1>404 Not Found</h1></body></html>";
  resp.body() = HttpBody{body_content.begin(), body_content.end()};
  return {};
  }) {}
// clang-format on

void Router::route(HttpRequest &req, HttpResponse &resp) {
  auto it = routing_table.lower_bound(req.requested_url);

  if (it != routing_table.end() && it->first == req.requested_url) {
    auto &[handler, _] = (*it).second;
    invoke_handler(handler, req, resp);
    return;
  }

  while (it != routing_table.begin()) {
    --it;
    auto &[route, record] = *it;
    auto &[handler, prefix_match] = record;
    if (prefix_match && req.requested_url.starts_with(route)) {
      req.requested_url = req.requested_url.substr(route.length());
      invoke_handler(handler, req, resp);
      return;
    }
  }

  invoke_handler(default_handler, req, resp);
}

void Router::invoke_handler(handler_function &handler, const HttpRequest &req, HttpResponse &res) {
  if (!handler) {
    return;
  }

  auto handler_result = handler(req, res);
  if (handler_result.has_value()) {
    return;
  }

  HandlerError err = handler_result.error();

  println(cerr, "Error occurred while invoking handler for route: {}, error: {}", req.requested_url, err.message());

  res.status() = err.status();
  res.headers().set("Content-Type", "text/html");
  string body_content = format("<html><body><h1>{} {}</h1></body></html>", (unsigned short)err.status(),
                               http_status_to_reason(err.status()).value_or("Custom status"));
  res.body() = HttpBody{body_content.begin(), body_content.end()};
}

void Router::add_handler(string route, handler_function handler_func, bool prefix_match) {
  routing_table.insert_or_assign(std::move(route), make_pair(std::move(handler_func), prefix_match));
}

void Router::set_default_handler(handler_function handler) { default_handler = std::move(handler); }

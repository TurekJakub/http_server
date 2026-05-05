/*
    This example doesn't have much in common with scope
    of this project, except of used technologies. Therfore
    this simple demo, that doesn't do much more then basically
    pinging Arch Linux servers, serves just as demonstration that
    project can be automaticaly build and linked with all it dependencies
    (Asio, OpenSSL), which can then use to perform basic network
    communication with TLS support
*/

#include <cstddef>
#include <format>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <print>
#include <sstream>
#include <string>

#include "asio/completion_condition.hpp"
#include "asio/error.hpp"
#include "asio/error_code.hpp"
#include "asio/ssl/verify_mode.hpp"
#include "asio/system_error.hpp"
#include "config_parser.h"
#include "http.h"
#include "server.h"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <thread>
#include <vector>

using namespace std;
using namespace asio;

void save_buffer_to_file(const vector<char> &buffer, const string &filename) {
  ofstream output_file(filename, ios::binary);
  if (!output_file.is_open()) {
    cerr << "Error: Could not open file for writing: " << filename << endl;
    return;
  }
  output_file.write(buffer.data(), buffer.size());
  std::cout << std::endl << "Successfully saved " << buffer.size() << " bytes to " << filename << std::endl;
}

int ping() {
  const string host = "httpbin.org";         //"fleet.coprosys.cz"; // "www.archlinux.org";
  const string target = "/stream-bytes/100"; //"/tile/8/137/90.png";
  const std::string port = "443";

  try {
    io_context io_context;

    ssl::context ssl_context(ssl::context::tls_client);
    // Using system trusted certs does not reliably on all platforms,
    // so for purposes of this simple demo add root CA cert manually
    ssl_context.load_verify_file("../src/resources/amazon.pem");
    ssl_context.set_verify_mode(ssl::verify_peer);

    ip::tcp::resolver resolver(io_context);
    ip::tcp::socket socket(io_context);

    ssl::stream<ip::tcp::socket> ssl_stream(std::move(socket), ssl_context);
    if (!SSL_set_tlsext_host_name(ssl_stream.native_handle(), host.c_str())) {
      print("Failed to set SNI hostname.\n");
      return 1;
    }

    auto endpoints = resolver.resolve(host, port);
    print("Resolving {}...\n", host);

    connect(ssl_stream.lowest_layer(), endpoints);
    print("Connection established.\n");

    ssl_stream.handshake(ssl::stream_base::client);
    print("SSL handshake successful.");

    string request = format("GET {} HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n", target, host);

    write(ssl_stream, buffer(request));
    print("HTTP request sent.\n");

    asio::streambuf buffer;
    error_code err;
    while (true) {
      asio::read(ssl_stream, buffer, transfer_at_least(1), err);
      if (err == asio::error::eof) {
        break;
      } else if (err) {
        print("Error while reading server response: {}\n", system_error(err).what());
        return 1;
      }
    }

    HttpParser p;
    std::istream asio_stream(&buffer);
    auto r = p.parse_response(asio_stream);
    if (!r.has_value()) {
      println("{}", r.error());
      return 1;
    }

    println("\nResponse:\nStatus: {}\nHeaders:", (int)r.value().status);

    for (auto &&h : r.value().headers.get_all()) {
      println("{} : {}", h.first, h.second);
    }

    save_buffer_to_file(r.value().body, "/home/jakub/Dokumenty/turekja5/project/build/test_tile.bin");

    string response = string(buffers_begin(buffer.data()), buffers_end(buffer.data()));
    // print("Server response:\n\n{}\n", response);
  } catch (exception &err) {
    print("Error: {}\n", err.what());
    return 1;
  }

  return 0;
}

void test_parse() {
  string raw_request = "POST /api/data HTTP/1.1\r\n"
                       "Host: localhost\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-Length: 13\r\n"
                       "\r\n"
                       "Hello, world!";

  istringstream s(raw_request);

  HttpParser p;
  auto res = p.parse_request(s);
  if (!res) {
    println("{}", res.error());
  }

  println("Method:");
  std::visit(
      [](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
          std::cout << "Parsed custom method: " << arg << std::endl;
        } else {
          std::cout << "Parsed known method: " << typeid(T).name() << std::endl;
        }
      },
      res.value().method);

  println("Requested url: {}", res.value().requested_url);
  println("Headers: ");
  for (auto &&h : res.value().headers.get_all()) {
    println("{} : {}", h.first, h.second);
  }
  println("Body: {}", string(res.value().body.begin(), res.value().body.end()));
}

void test_handler(const HttpRequest &req, HttpResponse &resp) {
  auto _ = req;
  resp.status = 200;
  resp.headers.set("Content-Type", "text/html");
  resp.headers.set("Content-Length", "64");
  string body_val = "<!doctype html><html><body><h1>Hello, World !</h1></body></html>";
  resp.body = HttpBody{body_val.begin(), body_val.end()};
}

int main() {
  io_context io_context;

  auto guard = make_work_guard(io_context);

  auto config_result = ServerConfig::parse_from_file("../src/resources/config.toml");
  if(!config_result.has_value()){
    println(cerr,"{}", config_result.error());
    return 1;
  }

  HttpServer s(io_context, config_result.value());
  s.add_handler("/", test_handler);
  s.start();

  const unsigned int thread_count = max(1u, config_result->max_thread_count);
  vector<std::thread> thread_pool;
  thread_pool.reserve(thread_count);

  for (size_t i = 0; i < thread_count; ++i) {
    thread_pool.emplace_back([&io_context]() { io_context.run(); });
  }

  for (auto &t : thread_pool) {
    if (t.joinable()) {
      t.join();
    }
  }
}
#include <cstddef>
#include <format>
#include <iostream>
#include <print>
#include <string>

#include "../libs/http/server.h"
#include "config_parser.h"
#include "static_files_handler.h"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <thread>
#include <vector>

using namespace std;
using namespace asio;

void test_handler(const HttpRequest &req, HttpResponse &resp) {
  auto _ = req;
  resp.status() = HttpStatus::OK;
  resp.headers().set("Content-Type", "text/html");
  resp.headers().set("Content-Length", "64");
  string body_val = "<!doctype html><html><body><h1>Hello, World !</h1></body></html>";
  resp.body() = HttpBody{body_val.begin(), body_val.end()};
}

int main() {
  io_context io_context;

  auto guard = make_work_guard(io_context);

  auto config_result = ServerConfig::parse_from_file("../resources/config.toml");
  if (!config_result.has_value()) {
    println(cerr, "{}", config_result.error());
    return 1;
  }

  HttpServer s(io_context, config_result.value());
  s.add_handler("/api", test_handler);

  if (config_result->static_files_source_dir.has_value()) {
    StaticFileHandler handler(*(config_result->static_files_source_dir));
    s.add_handler("/", handler, true);
  }

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
#include <format>
#include <iostream>
#include <print>
#include <string>

#include "static_file_server.h"

using namespace std;
using namespace http_server::http;

int main(int argc, char **argv) {
  vector<string> args(argv + 1, argv + argc);

  auto server_init_result = StaticFileServer::init(args);

  if (!server_init_result.has_value()) {
    println(cerr, "{}", server_init_result.error());
    return 1;
  }

  server_init_result->serve();

  return 0;
}
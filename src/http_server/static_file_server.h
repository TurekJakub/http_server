#ifndef STATIC_FILE_SERVER_H

#define STATIC_FILE_SERVER_H

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include <http_server/server.h>
#include "config_parser.h"

class StaticFileServer {
public:
  StaticFileServer(ServerConfig config) : http_server(config.http_server_config), config(std::move(config)) {};
  void serve();
  static std::expected<StaticFileServer, std::string> init(const std::vector<std::string> &cmd_args);
  private:
  static constexpr std::string_view default_config = "../resources/config.toml";
  http_server::http::HttpServer http_server;
  ServerConfig config;
};

#endif
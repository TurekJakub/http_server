#ifndef CONFIG_PARSER_H

#define CONFIG_PARSER_H

#include <expected>
#include <istream>
#include <optional>
#include <string>

#include "../libs/http/server.h"

class ServerConfig {
public:
  http_server::http::HttpServerConfig http_server_config{};

  std::optional<std::string> static_files_source_dir = {std::nullopt};
  std::optional<std::string> http_404_page = {std::nullopt};

  static std::expected<ServerConfig, std::string> parse(std::istream &input);
  static std::expected<ServerConfig, std::string> parse_from_file(const std::string &config_path);

private:
};

#endif
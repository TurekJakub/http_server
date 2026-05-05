#ifndef CONFIG_PARSER_H

#define CONFIG_PARSER_H
#define TOML_EXCEPTIONS 0

#include <expected>
#include <istream>
#include <optional>
#include <string>

#include <toml++/toml.hpp>

class ServerConfig {
public:
  unsigned short port = {443};
  std::string cert_path = {""};
  std::string private_key_path = {""};

  std::optional<std::string> static_files_source_dir = {std::nullopt};
  std::optional<std::string> http_404_page = {std::nullopt};

  unsigned int max_thread_count = {1};

  static std::expected<ServerConfig, std::string> parse(std::istream &input);
  static std::expected<ServerConfig, std::string> parse_from_file(const std::string &config_path);

private:
  static ServerConfig parse_impl(const toml::table &config_toml);
};

#endif
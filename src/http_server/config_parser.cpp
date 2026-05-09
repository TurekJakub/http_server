#define TOML_EXCEPTIONS 0

#include <expected>
#include <format>
#include <istream>
#include <optional>
#include <string>
#include <thread>

#include "config_parser.h"

#include <toml++/toml.hpp>

using namespace std;

namespace  {
  ServerConfig parse_cfg(const toml::table &config_toml) {
    ServerConfig cfg;
    cfg.http_server_config.port = config_toml["connection"]["port"].value_or(cfg.http_server_config.port);
    cfg.http_server_config.cert_path = config_toml["connection"]["certificate_path"].value_or(cfg.http_server_config.cert_path);
    cfg.http_server_config.private_key_path = config_toml["connection"]["private_key_path"].value_or(cfg.http_server_config.private_key_path);
    
    cfg.static_files_source_dir = config_toml["served_content"]["static_files_source_dir"].value<string>();
    cfg.http_404_page = config_toml["served_content"]["http_404_error_page"].value<string>();
    
    cfg.http_server_config.max_thread_count = config_toml["performance"]["max_thread_count"].value_or(std::thread::hardware_concurrency());
    
    return cfg;
  }
  
  expected<ServerConfig, string> parse_from_file_impl(const std::string &config_path) {
    auto parse_result = toml::parse_file(config_path);
    if (parse_result.failed()) {
      return unexpected(format("Parsing config file: {} failed with error: {}", config_path, parse_result.error().description()));
    }
    return parse_cfg(parse_result.table());
  }
  
  expected<ServerConfig, string> parse_impl(std::istream &input) {
    auto parse_result = toml::parse(input);
    if (parse_result.failed()) {
      return unexpected(format("Parsing server config from input stream failed with error: {}", parse_result.error().description()));
    }
    return parse_cfg(parse_result.table());
  }
}

expected<ServerConfig, string> ServerConfig::parse(std::istream &input) {
  return parse_impl(input);
}

std::expected<ServerConfig, std::string> ServerConfig::parse_from_file(const std::string &config_path){
    return parse_from_file_impl(config_path);
}


#include "static_file_server.h"
#include "config_parser.h"
#include "handlers/static_files_handler.h"
#include "handlers/default_handler.h"

using namespace std;

void StaticFileServer::serve() {
  if (config.static_files_source_dir.has_value()) {
    http_server.add_handler("/", StaticFileHandler(*config.static_files_source_dir), true);
  }
  if (config.http_404_page.has_value()) {
    http_server.set_default_handler(DefaultHandler(*config.http_404_page));
  }

  http_server.start();
}

expected<StaticFileServer, string> StaticFileServer::init(const vector<string> &cmd_args){
    string config_path(default_config);
    bool found_cfg_flag = false;
    for(auto &&arg : cmd_args){
        if(found_cfg_flag){
          config_path = arg;
          break;
        }
        found_cfg_flag = arg == "--config";
    }

    auto result = ServerConfig::parse_from_file(config_path);
    if(!result.has_value()){
      return unexpected(result.error());
    }

    return {*result};
}
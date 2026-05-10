#include "default_handler.h"
#include <filesystem>

using namespace std;
using namespace http_server::http;

expected<void, HandlerError> DefaultHandler::operator()(const HttpRequest &req, HttpResponse &res) const {
  auto &_ = req;

  if(req.method != GET){
    return unexpected(HandlerError("Only GET requests are allowed", http_server::http::HttpStatus::MethodNotAllowed));
  }

  if (!filesystem::exists(page_path)) {
    return unexpected(HandlerError("File specified by 'http_404_error_page' in config file not found", HttpStatus::NotFound));
  }

  auto result = serve_file(page_path, res);
  if (!result.has_value()) {
    return unexpected(HandlerError(result.error()));
  }
  return {};
}
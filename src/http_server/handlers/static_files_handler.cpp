#include <filesystem>
#include <format>
#include <string>

#include "static_files_handler.h"

using namespace std;
using namespace std::filesystem;
using namespace http_server::http;

expected<void, HandlerError> StaticFileHandler::operator()(const HttpRequest &req, HttpResponse &res) const {
  if(req.method != GET){
      return unexpected(HandlerError("Only GET requests are allowed", http_server::http::HttpStatus::MethodNotAllowed));
  }

  filesystem::path path = (source_dir / filesystem::path(req.requested_url).relative_path()).lexically_normal();

  if (filesystem::is_directory(path)) {
        path /= "index.html";
  }

  if (!filesystem::exists(path)) {
    return unexpected(HandlerError(format("Requested file does not exists, requested resource: {}", req.requested_url), HttpStatus::NotFound));
  }

  if (!is_child(source_dir, path)) {
    return unexpected(HandlerError{
        format("Requested resource is invalid, points outside specified static files dir, file path: {}, requested resource: {}",
               path.string(), req.requested_url),
        HttpStatus::Forbidden});
  }

  if (!filesystem::is_regular_file(path)) {
    return unexpected(HandlerError{
        format("Requested file does not exist - it is dir, dir path: {}, requested resource: {}", path.string(), req.requested_url), HttpStatus::NotFound});
  }

  auto server_file_result = serve_file(path.string(), res);
  if (!server_file_result.has_value()) {
    return unexpected(HandlerError(server_file_result.error()));
  }

  return {};
}

bool StaticFileHandler::is_child(const path &parent, const path &to_check) const {
  try {
    path parent_canonical = canonical(parent);
    path to_check_canonical = canonical(to_check);

    auto [parent_it, _] = std::ranges::mismatch(parent_canonical, to_check_canonical);

    return parent_it == parent_canonical.end();

  } catch (filesystem_error &err) {
    return false;
  }
}
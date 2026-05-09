#include "static_files_handler.h"
#include "../libs/http/mime_type.h"
#include "../libs/utils/fs_utils.h"
#include <filesystem>
#include <format>
#include <string>

using namespace std;

expected<void, HandlerError> StaticFileHandler::operator()(const HttpRequest &req, HttpResponse &res) const {
  filesystem::path path = (source_dir / filesystem::path(req.requested_url).relative_path()).lexically_normal();

  if (filesystem::is_directory(path)) {
        path /= "index.html";
  }

  if (!filesystem::exists(path)) {
    return unexpected(HandlerError(format("Requested file does not exists, requested resource: {}", req.requested_url), HttpStatus::NotFound));
  }

  if (!fsutils::is_child(source_dir, path)) {
    return unexpected(HandlerError{
        format("Requested resource is invalid, points outside specified static files dir, file path: {}, requested resource: {}",
               path.string(), req.requested_url),
        HttpStatus::Forbidden});
  }

  if (!filesystem::is_regular_file(path)) {
    return unexpected(HandlerError{
        format("Requested file does not exist - it is dir, dir path: {}, requested resource: {}", path.string(), req.requested_url), HttpStatus::NotFound});
  }

  res.status() = HttpStatus::OK;
  string mime(mimetypes::get_mime_for_file(path).value_or("application/octet-stream"));
  res.headers().set("Content-Type", mime);
  res.headers().set("X-Content-Type-Options", "nosniff");

  auto read_res = fsutils::read_file_to_buffer(path, res.body());
  if (!read_res.has_value()) {
    return unexpected(HandlerError(read_res.error()));
  }

  res.headers().set("Content-Length", to_string(res.body().size()));

  return {};
}
#ifndef MIME_TYPE_H

#define MIME_TYPE_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace http_server::mimetypes {
struct MimeMapping {
public:
  std::string_view ext;
  std::string_view mime_type;
};

std::optional<std::string_view> get_mime_for_ext(const std::string &ext);
std::optional<std::string_view> get_mime_for_file(const std::filesystem::path &file_path);
} // namespace mimetypes

#endif
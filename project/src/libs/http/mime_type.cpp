#include "mime_type.h"
#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

using namespace mimetypes;
using namespace std;

optional<string_view> mimetypes::get_mime_for_ext(const std::string &ext) {
    // Mapping for most common file extensions to corresponding MIME type
    // NOTE: when modifing this mapping keep items in alphabetical order 
    // doing otherwise would break binary seach bellow 
  static constexpr std::array<MimeMapping, 27> mime_map = {
      {
        {".bin", "application/octet-stream"},
       {".css", "text/css"},
       {".gif", "image/gif"},
       {".gz", "application/gzip"},
       {".htm", "text/html"},
       {".html", "text/html"},
       {".ico", "image/vnd.microsoft.icon"},
       {".jpeg", "image/jpeg"},
       {".jpg", "image/jpeg"},
       {".js", "application/javascript"},
       {".json", "application/json"},
       {".mp3", "audio/mpeg"},
       {".mp4", "video/mp4"},
       {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
       {".odt", "application/vnd.oasis.opendocument.text"},
       {".otf", "font/otf"},
       {".pdf", "application/pdf"},
       {".png", "image/png"},
       {".svg", "image/svg+xml"},
       {".ttf", "font/ttf"},
       {".txt", "text/plain"},
       {".wav", "audio/wav"},
       {".webp", "image/webp"},
       {".woff", "font/woff"},
       {".woff2", "font/woff2"},
       {".xml", "application/xml"},
       {".zip", "application/zip"},
       }
  };

  auto it = lower_bound(mime_map.begin(), mime_map.end(), ext, [](const MimeMapping &e, string_view s) { return e.ext < s; });
  if (it == mime_map.end()) {
    return nullopt;
  }

  return it->mime_type;
}

optional<string_view> mimetypes::get_mime_for_file(const std::filesystem::path &file_path) {
    return get_mime_for_ext(file_path.extension().string());
}

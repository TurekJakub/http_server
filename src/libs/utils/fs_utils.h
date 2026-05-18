#ifndef FS_UTILS_H

#define FS_UTILS_H

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace http_server::fsutils {
    std::expected<void, std::string> read_file_to_buffer(const std::filesystem::path &file_path, std::vector<char> &buffer);
    inline bool is_root_dir_path(const std::filesystem::path& path) {
        return !path.empty() && path.relative_path().empty() && path.has_root_directory();
    };
    std::expected<void, std::string> save_buffer_to_file(const std::vector<char> &buffer, const std::filesystem::path &save_to);
}

#endif
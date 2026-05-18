#ifndef STATIC_FILE_HANDLER_H

#define STATIC_FILE_HANDLER_H

#include <expected>
#include <filesystem>
#include <string>

#include <http_server/http.h>
#include <http_server/server.h>

class StaticFileHandler {
    public:
    StaticFileHandler(std::string source_dir_path) : source_dir(source_dir_path) {};
    std::expected<void, http_server::http::HandlerError> operator()(const http_server::http::HttpRequest &req, http_server::http::HttpResponse &res) const;
    private:
    bool is_child(const std::filesystem::path &parent, const std::filesystem::path &to_check) const;

    std::filesystem::path source_dir;
};

#endif
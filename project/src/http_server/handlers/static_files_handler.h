#ifndef STATIC_FILE_HANDLER_H

#define STATIC_FILE_HANDLER_H

#include <expected>
#include <filesystem>
#include <string>

#include "../../libs/http/http.h"
#include "../../libs/http/server.h"

class StaticFileHandler {
    public:
    StaticFileHandler(std::string source_dir_path) : source_dir(source_dir_path) {};
    std::expected<void, http_server::http::HandlerError> operator()(const http_server::http::HttpRequest &req, http_server::http::HttpResponse &res) const;
    private:
    std::filesystem::path source_dir;
};

#endif
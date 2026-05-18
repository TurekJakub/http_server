#ifndef DEFAULT_HANDLER_H

#define DEFAULT_HANDLER_H

#include <http_server/http.h>
#include <http_server/server.h>

#include <string>

class DefaultHandler {
    public:
    DefaultHandler(std::string default_page_path): page_path(default_page_path) {}; 
    std::expected<void, http_server::http::HandlerError> operator()(const http_server::http::HttpRequest &req, http_server::http::HttpResponse &res) const;
    private:
    std::string page_path;

};

#endif

/*
    This example doesn't have much in common with scope
    of this project, except of used technologies. Therfore
    this simple demo, that doesn't do much more then basically
    pinging Arch Linux servers, serves just as demonstration that
    project can be automaticaly build and linked with all it dependencies
    (Asio, OpenSSL), which can then use to perform basic network
    communication with TLS support
*/

#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <string>

#include "asio/completion_condition.hpp"
#include "asio/error.hpp"
#include "asio/error_code.hpp"
#include "asio/ssl/verify_mode.hpp"
#include "asio/system_error.hpp"
#include "parser.h"
#include <asio.hpp>
#include <asio/ssl.hpp>

using namespace std;
using namespace asio;

void save_buffer_to_file(const vector<char>& buffer, const string& filename) {
    ofstream output_file(filename, ios::binary);
    if (!output_file.is_open()) {
        cerr << "Error: Could not open file for writing: " << filename << endl;
        return;
    }
    output_file.write(buffer.data(), buffer.size());
    std::cout << "Successfully saved " << buffer.size() << " bytes to " << filename << std::endl;
}

int main() {
  const string host = "fleet.coprosys.cz"; // "www.archlinux.org";
  const string target = "/tile/8/137/89.png";
  const std::string port = "443";

  try {
    io_context io_context;

    ssl::context ssl_context(ssl::context::tls_client);
    // Using system trusted certs does not reliably on all platforms,
    // so for purposes of this simple demo add root CA cert manually
    ssl_context.load_verify_file("../src/resources/archlinux-org.pem");
    ssl_context.set_verify_mode(ssl::verify_peer);

    ip::tcp::resolver resolver(io_context);
    ip::tcp::socket socket(io_context);

    ssl::stream<ip::tcp::socket> ssl_stream(std::move(socket), ssl_context);
    if (!SSL_set_tlsext_host_name(ssl_stream.native_handle(), host.c_str())) {
      print("Failed to set SNI hostname.\n");
      return 1;
    }

    auto endpoints = resolver.resolve(host, port);
    print("Resolving {}...\n", host);

    connect(ssl_stream.lowest_layer(), endpoints);
    print("Connection established.\n");

    ssl_stream.handshake(ssl::stream_base::client);
    print("SSL handshake successful.");

    string request = format("GET {} HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n", target, host);

    write(ssl_stream, buffer(request));
    print("HTTP request sent.\n");

    asio::streambuf buffer;
    error_code err;
    while (true) {
      asio::read(ssl_stream, buffer, transfer_at_least(1), err);
      if (err == asio::error::eof) {
        break;
      } else if (err) {
        print("Error while reading server response: {}\n", system_error(err).what());
        return 1;
      }
    }

    
    HttpParser p;
    std::istream asio_stream(&buffer);
    auto r = p.parse_response(asio_stream);
    if(!r.has_value()){
      println("{}", r.error());
      return 1;
    }

    println("Response:\nStatus: {}\nHeaders:\n", (int) r.value().status);
    for (auto&& h : r.value().headers) {
        println("{} : {}", h.first, h.second);
    }

    save_buffer_to_file(r.value().body, "/home/jakub/Dokumenty/turekja5/project/build/test_tile.png");
    

    string response = string(buffers_begin(buffer.data()), buffers_end(buffer.data()));
    //print("Server response:\n\n{}\n", response);
  } catch (exception &err) {
    print("Error: {}\n", err.what());
    return 1;
  }

  return 0;
}
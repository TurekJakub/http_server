#include <filesystem>
#include <format>
#include <fstream>
#include <ios>

#include "fs_utils.h"

using namespace std::filesystem;
using namespace std;


expected<void, string> http_server::fsutils::read_file_to_buffer(const path &file_path, vector<char> &buffer) {
  ifstream file(file_path, ios_base::binary);

  if (!file) {
    return unexpected(format("Failed to open file", file_path.string()));
  }

  buffer.assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());

  file.close();

  return {};
}

 std::expected<void, std::string> http_server::fsutils::save_buffer_to_file(const vector<char> &buffer, const path &save_to){
  ofstream output_file(save_to, ios::binary);
  if (!output_file.is_open()) {
    return  unexpected(format("Failed to open target file: {}, when saving buffer to disk", save_to.string()));
  }

  output_file.write(buffer.data(), buffer.size());
  output_file.close();

  return {};
}
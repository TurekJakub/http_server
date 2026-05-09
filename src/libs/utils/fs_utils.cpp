#include <filesystem>
#include <format>
#include <fstream>
#include <ios>

#include "fs_utils.h"

using namespace std::filesystem;
using namespace std;

bool fsutils::is_child(const path &parent, const path &to_check) {
  try {
    path parent_canonical = canonical(parent);
    path to_check_canonical = canonical(to_check);

    auto [parent_it, _] = std::ranges::mismatch(parent_canonical, to_check_canonical);

    return parent_it == parent_canonical.end();

  } catch (filesystem_error &err) {
    return false;
  }
}

expected<void, string> fsutils::read_file_to_buffer(const path &file_path, vector<char> &buffer) {
  ifstream file(file_path, ios_base::binary);

  if (!file) {
    return unexpected(format("Failed to open file", file_path.string()));
  }

  buffer.assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());

  file.close();

  return {};
}

 std::expected<void, std::string> fsutils::save_buffer_to_file(const vector<char> &buffer, const path &save_to){
  ofstream output_file(save_to, ios::binary);
  if (!output_file.is_open()) {
    return  unexpected(format("Failed to open target file: {}, when saving buffer to disk", save_to.string()));
  }

  output_file.write(buffer.data(), buffer.size());
  output_file.close();

  return {};
}
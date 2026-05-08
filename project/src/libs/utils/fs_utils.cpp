#include <filesystem>
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

  buffer.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

  file.close();

  return {};
}
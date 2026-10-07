// main.cpp
// currently testing things

/*
 * pipeline input:
 * website directories for static, assets, styles, writings (md files),
 * writing_templates (html template files for writings)
 * specify where the feed.xml is
 * probably other things too
 *
 */

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

// ensure it ends with a '/'
#define SOURCE_PATH "test/"

bool path_exists(const fs::path &p, fs::file_status s = fs::file_status{}) {
  if (fs::status_known(s) ? fs::exists(s) : fs::exists(p))
    return true;

  std::cout << "filepath " << p << " not found!" << std::endl;
  return false;
}

int main() {
  // TODO: add command line arguments when needed

  // First check for the website directories specified in the config
  const fs::path source_path{SOURCE_PATH};
  if (!(path_exists(source_path)))
    return 0;

  // create dir for output
  const fs::path out_path{std::string(SOURCE_PATH) + "out/"};
  fs::create_directory(out_path);

  // TODO: Should be wrapping these in try/catch statements
  const fs::path static_path{std::string(SOURCE_PATH) + "static/"};
  if (!(path_exists(static_path)))
    return 0;

  const fs::path assets_path{std::string(SOURCE_PATH) + "assets/"};
  if (!(path_exists(assets_path)))
    return 0;

  const fs::path assets_out_path{std::string(SOURCE_PATH) + "out/assets"};

  fs::copy(static_path, out_path,
           fs::copy_options::recursive | fs::copy_options::update_existing);

  fs::copy(assets_path, assets_out_path,
           fs::copy_options::recursive | fs::copy_options::update_existing);

  // next is generation
  // start by collecting metadata for each file in writings
  // TODO: make struct with the metadata required: date, title, url, headings []
  const fs::path writings_path{std::string(SOURCE_PATH) + "writings/"};
  const fs::path writings_out_path{std::string(SOURCE_PATH) + "out/writings/"};
  for (const auto &entry : fs::directory_iterator(writings_path)) {
    std::cout << entry << std::endl;
  }
}

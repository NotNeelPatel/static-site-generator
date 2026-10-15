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

#include "md4c-html.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct ArticleMetadata {
  std::string date;
  std::string title;
  std::string filename;
  std::vector<std::string> headings; // for table of contents
};

// TODO: make struct with the metadata required: date, title, url, headings []

namespace fs = std::filesystem;

// ensure it ends with a '/'
#define SOURCE_PATH "/home/neel/projects/notneelpatel.github.io_test/"

bool path_exists(const fs::path &p, fs::file_status s = fs::file_status{}) {
  if (fs::status_known(s) ? fs::exists(s) : fs::exists(p))
    return true;

  std::cout << "filepath " << p << " not found!" << std::endl;
  return false;
}

static void append_html(const MD_CHAR *data, MD_SIZE size, void *userdata) {
  auto *output = static_cast<std::string *>(userdata);
  output->append(data, static_cast<std::size_t>(size));
}

int main() {
  // TODO: add command line arguments when needed

  // First check for the website directories specified in the config
  const fs::path source_path{SOURCE_PATH};
  if (!(path_exists(source_path)))
    return 1;

  // create dir for output
  const fs::path out_path{std::string(SOURCE_PATH) + "out/"};
  fs::create_directory(out_path);

  // TODO: Should be wrapping these in try/catch statements
  const fs::path static_path{std::string(SOURCE_PATH) + "static/"};
  if (!(path_exists(static_path)))
    return 1;

  const fs::path assets_path{std::string(SOURCE_PATH) + "assets/"};
  if (!(path_exists(assets_path)))
    return 1;

  const fs::path assets_out_path{std::string(SOURCE_PATH) + "out/assets"};

  fs::copy(static_path, out_path,
           fs::copy_options::recursive | fs::copy_options::update_existing);

  fs::copy(assets_path, assets_out_path,
           fs::copy_options::recursive | fs::copy_options::update_existing);

  // next is generation
  // start by collecting metadata for each file in writings
  const fs::path writings_path{std::string(SOURCE_PATH) + "writings/"};
  const fs::path writings_out_path{std::string(SOURCE_PATH) + "out/writings/"};
  // create the writings directory
  fs::create_directory(writings_out_path);

  for (const auto &entry : fs::directory_iterator(writings_path)) {
    // std::cout << entry << std::endl;
    std::ifstream md_file(entry.path());
    const std::string markdown((std::istreambuf_iterator<char>(md_file)),
                               std::istreambuf_iterator<char>());
    fs::path html_filename(entry.path().filename());
    // parse through metadata, will do later.
    struct ArticleMetadata metadata;
    metadata.filename =
        html_filename.replace_extension(".html").generic_string();

    std::string html;
    const int result =
        md_html(markdown.data(), static_cast<MD_SIZE>(markdown.size()),
                append_html, &html, MD_DIALECT_COMMONMARK, 0);

    if (result != 0) {
      return 1;
    }

    const fs::path html_writing_out_path{
        std::string(writings_out_path.generic_string()) + metadata.filename};

    std::cout << (writings_out_path.generic_string() + metadata.filename)
              << std::endl;
    std::ofstream myfile(html_writing_out_path);
    if (myfile.is_open()) {
      myfile << html;
      myfile.close();

    } else
      std::cout << "cant open file" << std::endl;

    // std::cout << markdown << '\n';
    // std::cout << html << '\n';
  }
  return 0;
}

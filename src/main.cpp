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
#include <cctype>
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

const std::string target_content = "{{ article_content }}";
// ensure it ends with a '/'
#define SOURCE_PATH "/home/neel/projects/notneelpatel.github.io/"

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

std::string clean_for_id(std::string title) {
  std::string cleanTitle;
  const std::string specialChars = "\"'/:;,.?!<>#@$%^&*()_+=\\";
  for (char c : title) {
    if (specialChars.find(c) != std::string::npos) {
      continue;
    } else if (std::isspace(c)) {
      cleanTitle += "-";
    } else {
      cleanTitle += (char)std::tolower(c);
    }
    c++; // ...say that again?
  }
  return cleanTitle;
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

  const fs::path templates_path{std::string(SOURCE_PATH) +
                                "templates/writing.html"};

  std::string entries_list;
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

    size_t pos;
    size_t pos2;

    pos = html.find("<h1>") + 4;
    pos2 = html.find("</h1>");
    metadata.title = html.substr(pos, pos2 - pos);
    html.replace(pos - 4, metadata.title.length() + 9, "");
    std::cout << "---" << std::endl;
    std::cout << metadata.title << std::endl;

    pos = html.find("<h3>") + 4;
    pos2 = html.find("</h3>");
    metadata.date = html.substr(pos, pos2 - pos);
    html.replace(pos - 4, metadata.date.length() + 9, "");

    const fs::path html_writing_out_path{
        std::string(writings_out_path.generic_string()) + metadata.filename};

    std::ifstream html_template_path(templates_path);
    std::string html_full((std::istreambuf_iterator<char>(html_template_path)),
                          std::istreambuf_iterator<char>());

    // augment headers for table of contents
    char hType;
    int toc_h = 0;
    std::string header_title;
    std::string header_id;
    pos = 0;
    pos2 = 0;

    std::string toc_html = "";
    while ((pos = html.find("<h", pos)) != std::string::npos) {
      hType = html[pos + 2];

      if (hType < '3' || hType > '5') {
        pos += 2;
        continue;
      }

      pos += 4;
      pos2 = html.find(std::string("</h") + hType, pos);
      header_title = html.substr(pos, (pos2 - pos));
      header_id = clean_for_id(header_title);
      std::string header_id_insert = " id=\"" + header_id + "\"";
      html.insert(pos - 1, header_id_insert);
      toc_h = hType - '0' + 1;

      pos += header_id_insert.size() + 9;
      toc_html +=
          ("<h" + std::to_string(toc_h) + "><a href=\"#" + header_id + "\">" +
           header_title + "</a></h" + std::to_string(toc_h) + ">\n");

      std::cout << "toc! " << toc_html << std::endl;
    }

    const std::string target_title = "{{ article_title }}";
    const std::string target_date = "{{ article_date }}";
    const std::string target_content = "{{ article_content }}";
    const std::string target_toc = "{{ article_toc }}";
    pos = html_full.find(target_content);
    html_full.replace(pos, target_content.length(), html);

    pos = html_full.find(target_title);
    html_full.replace(pos, target_title.length(), metadata.title);

    pos = html_full.find(target_date);
    html_full.replace(pos, target_date.length(), metadata.date);

    pos = html_full.find(target_toc);
    html_full.replace(pos, target_toc.length(), toc_html);

    std::ofstream html_file(html_writing_out_path);
    if (html_file.is_open()) {
      html_file << html_full;
      html_file.close();

    } else
      std::cout << "cant open file" << std::endl;

    // std::cout << markdown << '\n';
    // std::cout << html << '\n';
    entries_list += "<p>" + metadata.date + " <a href=\"" + "./writings/" +
                    html_filename.generic_string() + "\">" + metadata.title +
                    "</a></p>";
  }

  // deal with writings page

  const fs::path template_list_path{std::string(SOURCE_PATH) +
                                    "templates/writing_list.html"};
  const fs::path writings_list_path{std::string(out_path) + "writings.html"};

  std::ifstream html_template_list_path(template_list_path);
  std::string html_template_writing(
      (std::istreambuf_iterator<char>(html_template_list_path)),
      std::istreambuf_iterator<char>());

  size_t pos = html_template_writing.find(target_content);
  html_template_writing.replace(pos, target_content.length(), entries_list);

  std::ofstream html_template_writing_file(writings_list_path);
  if (html_template_writing_file.is_open()) {
    html_template_writing_file << html_template_writing;
    html_template_writing_file.close();
  }

  return 0;
}

// main.cpp

#include "md4c-html.h"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct ArticleMetadata {
  std::string date;
  std::string title;
  std::string filename;
  std::vector<std::string> entries_list; // for table of contents
};

namespace fs = std::filesystem;

const std::string target_title = "{{ article_title }}";
const std::string target_date = "{{ article_date }}";
const std::string target_content = "{{ article_content }}";
const std::string target_toc = "{{ article_toc }}";
const std::string target_last_updated = "{{ last_updated }}";
const std::string target_latest = "{{ latest }}";
// ensure it ends with a '/'
#define SOURCE_PATH "/home/neel/projects/notneelpatel.github.io/"

bool pathExists(const fs::path &p, fs::file_status s = fs::file_status{}) {
  if (fs::status_known(s) ? fs::exists(s) : fs::exists(p))
    return true;

  std::cout << "filepath " << p << " not found!" << std::endl;
  return false;
}

static void appendHtml(const MD_CHAR *data, MD_SIZE size, void *userdata) {
  auto *output = static_cast<std::string *>(userdata);
  output->append(data, static_cast<std::size_t>(size));
}

std::string cleanForId(std::string title) {
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

int copyDir(std::string dir_name, std::string out_name = "same") {
  if (dir_name.back() != '/') {
    dir_name.push_back('/');
  }
  if (out_name == "same") {
    out_name = dir_name;
  }
  if (!(out_name.empty()) && (out_name.back() != '/')) {
    out_name.push_back('/');
  }
  const fs::path dir_path{std::string(SOURCE_PATH) + dir_name};
  if (!(pathExists(dir_path)))
    return 1;

  const fs::path out_path{std::string(SOURCE_PATH) + "out/" + out_name};

  fs::copy(dir_path, out_path,
           fs::copy_options::recursive | fs::copy_options::update_existing);
  return 0;
}

std::string readFile(std::string path_string) {
  const fs::path path{std::string(SOURCE_PATH) + path_string};
  std::ifstream read_file(path);
  std::string data((std::istreambuf_iterator<char>(read_file)),
                   std::istreambuf_iterator<char>());
  return data;
}

int writeFile(std::string data, fs::path path) {
  std::ofstream file(path);
  if (file.is_open()) {
    file << data;
    file.close();
  }
  return 0;
}

ArticleMetadata processArticle(fs::directory_entry entry) {
  const fs::path writings_out_path{std::string(SOURCE_PATH) + "out/writings/"};
  fs::create_directory(writings_out_path);

  const fs::path templates_path{std::string(SOURCE_PATH) +
                                "templates/writing.html"};

  std::ifstream md_file(entry.path());
  const std::string markdown((std::istreambuf_iterator<char>(md_file)),
                             std::istreambuf_iterator<char>());
  fs::path html_filename(entry.path().filename());
  // parse through metadata, will do later.
  struct ArticleMetadata metadata;
  metadata.filename = html_filename.replace_extension(".html").generic_string();

  std::string html;
  const int result =
      md_html(markdown.data(), static_cast<MD_SIZE>(markdown.size()),
              appendHtml, &html, MD_DIALECT_COMMONMARK, 0);

  if (result != 0) {
    std::cout << "MD4C HTML error, exiting" << std::endl;
    // TODO: add null
  }

  size_t pos;
  size_t pos2;

  pos = html.find("<h1>") + 4;
  pos2 = html.find("</h1>");
  metadata.title = html.substr(pos, pos2 - pos);
  html.replace(pos - 4, metadata.title.length() + 9, "");

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
  char h_type;
  int toc_h = 0;
  std::string header_title;
  std::string header_id;
  pos = 0;
  pos2 = 0;

  std::string toc_html = "";
  while ((pos = html.find("<h", pos)) != std::string::npos) {
    h_type = html[pos + 2];

    if (h_type < '3' || h_type > '5') {
      pos += 2;
      continue;
    }

    pos += 4;
    pos2 = html.find(std::string("</h") + h_type, pos);
    header_title = html.substr(pos, (pos2 - pos));
    header_id = cleanForId(header_title);
    std::string header_id_insert = " id=\"" + header_id + "\"";
    html.insert(pos - 1, header_id_insert);
    toc_h = h_type - '0' + 1;

    pos += header_id_insert.size() + 9;
    toc_html +=
        ("<h" + std::to_string(toc_h) + "><a href=\"#" + header_id + "\">" +
         header_title + "</a></h" + std::to_string(toc_h) + ">\n");
  }

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
  return metadata;
}

int main() {
  // First check for the website directories specified in the config
  const fs::path source_path{SOURCE_PATH};
  if (!(pathExists(source_path)))
    return 1;

  // create dir for output
  const fs::path out_path{std::string(SOURCE_PATH) + "out/"};
  fs::create_directory(out_path);

  // Copy static data
  copyDir("static/", "");
  copyDir("assets/");
  copyDir("styles/");

  // Generate article posts in writings/ dir
  //
  const fs::path writings_path{std::string(SOURCE_PATH) + "writings/"};
  std::string entries_list;
  for (const auto &entry : fs::directory_iterator(writings_path)) {
    struct ArticleMetadata metadata = processArticle(entry);
    entries_list += "<p>" + metadata.date + " <a href=\"" + "./writings/" +
                    metadata.filename + "\">" + metadata.title + "</a></p>";
  }
  // Construct Writings page (will completely redo later anyways)
  const fs::path writings_list_path{std::string(out_path) + "writings.html"};

  std::string html_template_writing = readFile("templates/writing_list.html");

  size_t pos = html_template_writing.find(target_content);
  html_template_writing.replace(pos, target_content.length(), entries_list);

  writeFile(html_template_writing, writings_list_path);

  // Construct index.html (add last updated and latest article)
  const fs::path home_out_path{std::string(out_path) + "index.html"};

  std::string index_html = readFile("templates/index.html");

  time_t timestamp;
  time(&timestamp);

  std::tm *local_time = std::localtime(&timestamp);

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%a %b %d %Y", local_time);
  std::string last_updated = buffer;

  std::transform(last_updated.begin(), last_updated.end(), last_updated.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  pos = index_html.find(target_last_updated);
  index_html.replace(pos, target_last_updated.length(), last_updated);

  pos = index_html.find(target_latest);
  index_html.replace(pos, target_latest.length(), "/writings/");

  writeFile(index_html, home_out_path);

  return 0;
}

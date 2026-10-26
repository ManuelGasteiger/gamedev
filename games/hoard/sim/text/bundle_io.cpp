#include "bundle_io.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include "ages/json.h"

namespace text {

namespace fs = std::filesystem;
using nlohmann::json;

bool bundle_dir_json(const std::string& dir, const std::string& lang, std::string& out, std::string& err) {
    std::error_code ec;
    std::vector<fs::path> files;
    for (auto& e : fs::directory_iterator(fs::path(dir) / lang, ec))
        if (e.path().extension() == ".json") files.push_back(e.path());
    if (ec) { err = "cannot list " + dir + "/" + lang; return false; }
    std::sort(files.begin(), files.end());
    json b = json::object();
    b["lang"] = lang;
    for (auto& f : files) {
        std::ifstream in(f);
        std::ostringstream ss;
        ss << in.rdbuf();
        json v = json::parse(ss.str(), nullptr, false);
        if (v.is_discarded()) { err = "invalid json in " + f.string(); return false; }
        std::string stem = f.filename().string();
        b[stem.substr(0, stem.size() - 5)] = v;
    }
    out = b.dump();
    return true;
}

bool bundles_dir_json(const std::string& text_dir, std::map<std::string, std::string>& out, std::string& err) {
    std::error_code ec;
    for (auto& e : fs::directory_iterator(text_dir, ec)) {
        if (!e.is_directory()) continue;
        std::string lang = e.path().filename().string();
        if (!bundle_dir_json(text_dir, lang, out[lang], err)) return false;
    }
    if (ec) { err = "cannot list " + text_dir; return false; }
    return true;
}

}

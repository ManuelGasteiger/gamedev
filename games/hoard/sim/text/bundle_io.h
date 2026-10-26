#pragma once
#include <map>
#include <string>

namespace text {

// data/text/<lang>/*.json merged into one bundle text, {"lang": lang, "<file stem>": <file>}; not in the wasm build
bool bundle_dir_json(const std::string& dir, const std::string& lang, std::string& out, std::string& err);

// every language directory under data/text
bool bundles_dir_json(const std::string& text_dir, std::map<std::string, std::string>& out, std::string& err);

}

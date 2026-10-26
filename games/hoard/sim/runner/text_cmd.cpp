#include "text_cmd.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include "text/batch.h"
#include "text/bundle_io.h"

using nlohmann::json;

int render_batch_cmd(const std::string& data, const std::string& file) {
    std::map<std::string, std::string> texts;
    std::string err;
    text::Bundles all;
    if (!text::bundles_dir_json(data + "/text", texts, err) || !text::bundles_from_json(texts, all, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    std::ifstream in(file);
    std::ostringstream ss;
    ss << in.rdbuf();
    json jobs = json::parse(ss.str(), nullptr, false);
    if (jobs.is_discarded() || !jobs.is_array()) { std::fprintf(stderr, "%s: not a json array\n", file.c_str()); return 1; }
    std::printf("%s\n", text::render_batch(all, jobs).dump(1).c_str());
    return 0;
}

int text_bundle_cmd(const std::string& data, const std::string& lang) {
    std::string out, err;
    if (!text::bundle_dir_json(data + "/text", lang, out, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    std::printf("%s\n", out.c_str());
    return 0;
}

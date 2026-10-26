#pragma once
#include <string>

// --render-batch FILE: renders the jobs of FILE (text/batch.h) with every language of DATA/text, prints the result json
int render_batch_cmd(const std::string& data, const std::string& file);

// --text-bundle LANG: prints the bundle of DATA/text/LANG as the build stages it (text.<lang>.json)
int text_bundle_cmd(const std::string& data, const std::string& lang);

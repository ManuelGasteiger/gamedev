#pragma once
#include <map>
#include <string>
#include "ages/json.h"

// item 19: the text commands of the wasm API. The shells pass text bundles with "load"; strings come back finished.
bool text_load(const std::map<std::string, std::string>& bundles, std::string& err);

// "ui" {lang}: every ui string of lang as a template; "render" {lang, key | template, ctx}; "render_batch" {jobs}
bool text_dispatch(const std::string& cmd, const nlohmann::json& in, nlohmann::json& out);

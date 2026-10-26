#pragma once
#include "ages/json.h"
#include "render.h"

namespace text {

// {"roles": {name: {kind, name, noun, sex, gender, fields{}, numbers{}}}, "vars": {}}
Ctx ctx_from_json(const nlohmann::json& j);

// [{"lang", "key" | "template", "ctx"}] -> {"renders": [{"text", "problems"}], "hash"}. Each var of a ctx is a clause
// template rendered first. The hash covers every text, so native and wasm compare in one line.
nlohmann::json render_batch(const Bundles& all, const nlohmann::json& jobs);

}

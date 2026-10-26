#include "text_api.h"
#include "text/batch.h"

using nlohmann::json;

namespace {

text::Bundles g_text;

json ui_strings(const std::string& lang) {
    json out = json::object();
    const text::Bundle* en = g_text.get("en");
    const text::Bundle* b = g_text.get(lang);
    if (g_text.release && en) out = text::part(*en, "ui");
    if (b) for (auto& [k, v] : text::part(*b, "ui").items()) out[k] = v;
    return out;
}

}

bool text_load(const std::map<std::string, std::string>& bundles, std::string& err) {
    return text::bundles_from_json(bundles, g_text, err);
}

bool text_dispatch(const std::string& cmd, const json& in, json& out) {
    if (cmd == "ui") {
        out = {{"ok", true}, {"lang", in.value("lang", std::string("en"))}, {"strings", ui_strings(in.value("lang", std::string("en")))}};
        return true;
    }
    if (cmd == "render") {
        json job = {{"lang", in.value("lang", std::string("en"))}, {"ctx", in.value("ctx", json::object())}};
        if (in.contains("key")) job["key"] = in["key"];
        else job["template"] = in.value("template", std::string());
        json r = text::render_batch(g_text, json::array({job}));
        out = r["renders"][0];
        out["ok"] = true;
        return true;
    }
    if (cmd == "render_batch") {
        out = text::render_batch(g_text, in.value("jobs", json::array()));
        out["ok"] = true;
        return true;
    }
    return false;
}

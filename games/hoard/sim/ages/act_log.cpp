#include "act_log.h"
#include "world_util.h"

namespace ages {

using nlohmann::json;

namespace {

const char* const kStructures[] = {"library", "temple", "foundry", "walls", "altar", "granary", "tower"};
const char* const kFlags[] = {"consent", "dreams", "skip", "reroll"};
const char* const kLoot[] = {"gold", "books", "artifacts", "art", "food"};

bool is_structure(const std::string& s) {
    for (const char* k : kStructures) if (s == k) return true;
    return false;
}

bool is_flag(const std::string& s) {
    for (const char* k : kFlags) if (s == k) return true;
    return false;
}

bool is_loot(const std::string& s) {
    for (const char* k : kLoot) if (s == k) return true;
    return false;
}

std::string arg(const Act& a, const char* k) {
    auto it = a.args.find(k);
    return it == a.args.end() ? "" : it->second;
}

double num(const Act& a, const char* k) {
    auto it = a.payload.find(k);
    return it == a.payload.end() ? 0 : it->second;
}

// the act's own field and its typed payload key name the same thing; whichever is set fills the other
void pair(std::string& field, std::map<std::string, std::string>& args, const char* key) {
    std::string& v = args[key];
    if (v.empty()) v = field;
    else if (field.empty()) field = v;
    if (v.empty()) args.erase(key);
}

std::string ref_kind(const World& w, const std::string& id) {
    if (find_faction(w, id)) return "faction";
    if (find_region(w, id)) return "region";
    if (find_person(w, id)) return "person";
    return "faction";
}

}

void act_payload_from(const json& j, Act& a) {
    if (j.contains("target")) {
        const json& t = j["target"];
        if (t.is_string()) a.target = t.get<std::string>();
        else if (t.is_object()) {
            if (t.contains("kind") && t["kind"].is_string()) a.target_kind = t["kind"].get<std::string>();
            if (t.contains("id") && t["id"].is_string()) a.target = t["id"].get<std::string>();
        }
    }
    if (j.contains("secondary") && j["secondary"].is_object()) {
        const json& s = j["secondary"];
        std::string kind = s.value("kind", ""), id = s.contains("id") && s["id"].is_string() ? s["id"].get<std::string>() : "";
        if (kind == "structure") a.item = id;
        else a.against = id;
    }
    auto take = [&](const json& obj) {
        if (!obj.is_object()) return;
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            const json& v = it.value();
            if (v.is_number()) a.payload[it.key()] = v.get<double>();
            else if (v.is_boolean()) a.payload[it.key()] = v.get<bool>() ? 1 : 0;
            else if (v.is_string()) a.args[it.key()] = v.get<std::string>();
            else if (v.is_array()) {
                std::string list;
                for (auto& x : v) if (x.is_string()) list += (list.empty() ? "" : ",") + x.get<std::string>();
                a.args[it.key()] = list;
            } else if (v.is_object()) {
                if (v.contains("id") && v["id"].is_string()) { a.args[it.key()] = v["id"].get<std::string>(); a.args[it.key() + "_kind"] = v.value("kind", ""); }
                else a.args[it.key()] = v.dump();
            }
        }
    };
    if (j.contains("payload")) take(j["payload"]);
    if (j.contains("args")) take(j["args"]);
}

void normalise_act(const World& w, Act& a) {
    a.magnitude = clamp01(a.magnitude);
    if (a.target_kind == "region") {
        a.region = a.target;
        const Region* r = find_region(w, a.region);
        a.target = r ? r->owner : "";
    } else if (a.target_kind == "person") {
        a.item = a.target;
        const Person* x = find_person(w, a.item);
        a.target = x ? x->faction : "";
    } else if (a.target_kind == "item" || a.target_kind == "hook" || a.target_kind == "scenario" || a.target_kind == "remnant") {
        a.item = a.target;
        a.target.clear();
    } else if (a.target_kind == "offer") {
        a.args["answers_offer"] = a.target;
        a.target.clear();
    } else if (a.target_kind == "lair") a.target.clear();
    if (a.type == "receive") pair(a.item, a.args, "outcome");
    else if (a.type == "demand") pair(a.item, a.args, "clause");
    else if (a.type == "exit") pair(a.item, a.args, "kind");
    else if (a.type == "take") pair(a.item, a.args, "item");
    else if (a.type == "show" && arg(a, "kind").empty()) a.args["kind"] = a.magnitude <= 0.1 ? "flyover" : (a.magnitude <= 0.3 ? "roar" : "land");
    else if (a.type == "miracle") {
        if (arg(a, "kind").empty()) a.args["kind"] = num(a, "terror") > 0 ? "terror" : "protector";
        else if (arg(a, "kind") == "terror") a.payload["terror"] = 1;
    }
}

json act_schema_json(const World& w, const Act& a, int id) {
    json j;
    j["id"] = id;
    j["year"] = a.year;
    j["type"] = a.type;
    j["source"] = a.source.empty() ? "raid" : a.source;
    if (!a.answers.empty()) j["answers"] = a.answers;
    j["magnitude"] = a.magnitude;
    std::string kind = a.target_kind;
    if (kind.empty()) kind = !a.target.empty() ? ref_kind(w, a.target) : (!a.region.empty() ? "region" : "");
    std::string tid = kind == "region" ? (a.region.empty() ? a.target : a.region)
                    : (kind == "person" || kind == "item" || kind == "hook" || kind == "scenario" || kind == "remnant") ? a.item
                    : kind == "offer" ? arg(a, "answers_offer") : kind == "lair" ? "lair" : a.target;
    if (!kind.empty() && !tid.empty()) j["target"] = {{"kind", kind}, {"id", tid}};
    if (!a.region.empty()) j["region"] = a.region;
    if (a.type == "burn" && is_structure(a.item)) j["secondary"] = {{"kind", "structure"}, {"id", a.item}};
    else if (!a.against.empty()) j["secondary"] = {{"kind", ref_kind(w, a.against)}, {"id", a.against}};
    json pl = json::object();
    bool loot = a.type == "take" || a.type == "give";
    for (auto& kv : a.payload) {
        if (loot && !is_loot(kv.first)) continue;
        if (is_flag(kv.first)) pl[kv.first] = kv.second > 0;
        else if (kv.first == "years") pl[kv.first] = static_cast<long long>(kv.second);
        else pl[kv.first] = kv.second;
    }
    for (auto& kv : a.args) {
        if (kv.first == "terms" || kv.first == "params" || kv.first.size() > 5 && kv.first.compare(kv.first.size() - 5, 5, "_kind") == 0) continue;
        if (loot && kv.first != "item" && kv.first != "region") continue;
        if (a.args.count(kv.first + "_kind")) pl[kv.first] = {{"kind", a.args.at(kv.first + "_kind").empty() ? ref_kind(w, kv.second) : a.args.at(kv.first + "_kind")}, {"id", kv.second}};
        else if (kv.first == "wake_on") {
            json list = json::array();
            for (size_t at = 0; at < kv.second.size();) {
                size_t comma = kv.second.find(',', at);
                list.push_back(kv.second.substr(at, comma == std::string::npos ? std::string::npos : comma - at));
                at = comma == std::string::npos ? kv.second.size() : comma + 1;
            }
            pl[kv.first] = list;
        } else pl[kv.first] = kv.second;
    }
    if (a.type == "kill") { pl.erase("soldiers"); if (num(a, "soldiers") > 0) pl["who"] = "army"; }
    if (a.type == "miracle") pl.erase("terror");
    if (a.type == "release") pl = json::object();
    if (!pl.empty() || loot) j["payload"] = pl;
    j["witnesses"] = a.witnesses;
    return j;
}

std::string act_log_json(const World& w) {
    json arr = json::array();
    for (size_t i = 0; i < w.acts.size(); ++i) {
        const Act& a = w.acts[i];
        if (a.type == "spell" || a.type == "wound") continue;   // internal: sponsor_remnant's old spell act, a passive wound
        arr.push_back(act_schema_json(w, a, static_cast<int>(i)));
    }
    return arr.dump(1);
}

}

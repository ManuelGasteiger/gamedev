#include "state_hash.h"
#include <cstdio>
#include "core/hash.h"
#include "json.h"

namespace ages {

nlohmann::json world_state_json(const World& w);   // serialise.cpp

namespace {

enum Tag : uint64_t { t_null = 1, t_bool, t_int, t_uint, t_float, t_str, t_array, t_object };

void walk(det::Hasher& h, const nlohmann::json& j) {
    using V = nlohmann::json::value_t;
    switch (j.type()) {
        case V::null: h.tag(t_null); break;
        case V::boolean: h.tag(t_bool); h.u64(j.get<bool>() ? 1 : 0); break;
        case V::number_integer: h.tag(t_int); h.i64(j.get<int64_t>()); break;
        case V::number_unsigned: h.tag(t_uint); h.u64(j.get<uint64_t>()); break;
        case V::number_float: h.tag(t_float); h.f64(j.get<double>()); break;
        case V::string: h.tag(t_str); h.str(j.get_ref<const std::string&>()); break;
        case V::array:
            h.tag(t_array);
            h.u64(j.size());
            for (auto& v : j) walk(h, v);
            break;
        case V::object:   // nlohmann keeps object keys sorted
            h.tag(t_object);
            h.u64(j.size());
            for (auto it = j.begin(); it != j.end(); ++it) { h.str(it.key()); walk(h, it.value()); }
            break;
        default: h.tag(0xff); break;
    }
}

}

uint64_t json_hash(const nlohmann::json& j) {
    det::Hasher h;
    walk(h, j);
    return h.done();
}

StateHash state_hash(const World& w) {
    nlohmann::json j = world_state_json(w);
    StateHash s;
    det::Hasher root;
    for (auto it = j.begin(); it != j.end(); ++it) {
        det::Hasher part;
        walk(part, it.value());
        uint64_t ph = part.done();
        s.parts.emplace_back(it.key(), ph);
        root.str(it.key());
        root.u64(ph);
    }
    s.root = root.done();
    return s;
}

std::string hash_hex(uint64_t h) {
    char buf[20];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(h));
    return buf;
}

std::string state_hash_json(const World& w) {
    StateHash s = state_hash(w);
    nlohmann::json j;
    j["year"] = w.year;
    j["root"] = hash_hex(s.root);
    nlohmann::json parts = nlohmann::json::object();
    for (auto& p : s.parts) parts[p.first] = hash_hex(p.second);
    j["parts"] = parts;
    return j.dump();
}

}

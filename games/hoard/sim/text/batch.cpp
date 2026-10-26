#include "batch.h"
#include "ages/state_hash.h"
#include "core/hash.h"

namespace text {

using nlohmann::json;

Ctx ctx_from_json(const json& j) {
    Ctx c;
    if (j.contains("roles"))
        for (auto it = j["roles"].begin(); it != j["roles"].end(); ++it) {
            const json& v = it.value();
            Entity e;
            e.kind = v.value("kind", std::string());
            e.name = v.value("name", std::string());
            e.noun = v.value("noun", std::string());
            e.sex = v.value("sex", std::string());
            e.gender = v.value("gender", std::string());
            if (v.contains("fields")) for (auto f = v["fields"].begin(); f != v["fields"].end(); ++f) e.fields[f.key()] = f.value().get<std::string>();
            if (v.contains("numbers")) for (auto n = v["numbers"].begin(); n != v["numbers"].end(); ++n) e.numbers[n.key()] = n.value().get<long long>();
            c.roles[it.key()] = e;
        }
    if (j.contains("vars")) for (auto it = j["vars"].begin(); it != j["vars"].end(); ++it) c.vars[it.key()] = it.value().get<std::string>();
    return c;
}

json render_batch(const Bundles& all, const json& jobs) {
    json out = json::array();
    det::Hasher h;
    for (auto& job : jobs) {
        std::vector<std::string> problems;
        Ctx c = ctx_from_json(job.value("ctx", json::object()));
        std::string lang = job.value("lang", std::string("en"));
        const Bundle* b = all.get(lang);
        if (b) for (auto& v : c.vars) v.second = render(*b, v.second, c, &problems);   // a var is a clause template itself
        std::string t;
        if (job.contains("key")) t = render_key(all, lang, job["key"].get<std::string>(), c, &problems);
        else {
            if (b) t = render(*b, job.value("template", std::string()), c, &problems);
            else problems.push_back("no bundle " + lang);
        }
        h.str(t);
        out.push_back({{"text", t}, {"problems", problems}});
    }
    return {{"renders", out}, {"hash", ages::hash_hex(h.done())}};
}

}

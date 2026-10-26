#include "names.h"

namespace text {

using nlohmann::json;

namespace {

bool ends_with(const std::string& s, const std::string& end) { return s.size() >= end.size() && s.compare(s.size() - end.size(), end.size(), end) == 0; }

}

std::string name_in_case(const json& names, const std::string& name, const std::string& cas) {
    auto rule = names.find(cas);
    if (cas.empty() || cas == "nom" || rule == names.end() || !rule->is_object()) return name;
    auto over = rule->find("overrides");
    if (over != rule->end() && over->contains(name)) return (*over)[name].get<std::string>();
    if (auto after = rule->find("apostrophe_after"); after != rule->end())
        for (auto& e : *after)
            if (e.is_string() && ends_with(name, e.get<std::string>())) return name + rule->value("apostrophe", std::string("'"));
    return name + rule->value("suffix", std::string());
}

}

#include "names.h"
#include <cctype>
#include <cstdio>
#include "world_util.h"

namespace ages {

static const NameTable* table(const std::map<std::string, NameTable>& m, const std::string& race) {
    auto it = m.find(race);
    if (it != m.end() && !it->second.a.empty() && !it->second.b.empty()) return &it->second;
    it = m.find("human");
    if (it != m.end() && !it->second.a.empty() && !it->second.b.empty()) return &it->second;
    return nullptr;
}

std::string new_faction_name(World& w, const Params& p, const std::string& race) {
    const NameTable* t = table(p.names.factions, race);
    if (t) {
        auto free_name = [&](std::string name) {
            for (auto& ch : name) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if (find_faction(w, name)) return std::string();
            for (auto& g : w.graveyard) if (g.id == name) return std::string();
            return name;
        };
        size_t na = t->a.size(), nb = t->b.size();
        for (int tries = 0; tries < 8; ++tries) {
            int ia = w.rng.range(0, static_cast<int>(na));
            int ib = w.rng.range(0, static_cast<int>(nb));
            std::string name = free_name(t->a[static_cast<size_t>(ia)] + t->b[static_cast<size_t>(ib)]);
            if (!name.empty()) return name;
        }
        // crowded valley: walk every pair, then every pair with a second ending, from a random start
        size_t start = static_cast<size_t>(w.rng.range(0, static_cast<int>(na * nb)));
        for (size_t k = 0; k < na * nb; ++k) {
            size_t i = (start + k) % (na * nb);
            std::string name = free_name(t->a[i / nb] + t->b[i % nb]);
            if (!name.empty()) return name;
        }
        for (size_t k = 0; k < na * nb * nb; ++k) {
            size_t i = (start + k) % (na * nb * nb);
            std::string name = free_name(t->a[i / (nb * nb)] + t->b[(i / nb) % nb] + t->b[i % nb]);
            if (!name.empty()) return name;
        }
    }
    char buf[32];
    std::snprintf(buf, sizeof buf, "%s_%d", race.c_str(), w.next_serial++);
    return buf;
}

std::string epithet_for(const Params& p, const Person& person) {
    const PersonTraits& t = person.traits;
    const char* best = nullptr;
    double top = p.c.epithet_trait;
    if (t.bold >= top) { top = t.bold; best = "bold"; }
    if (t.pious > top) { top = t.pious; best = "pious"; }
    if (t.greedy > top) { top = t.greedy; best = "greedy"; }
    if (t.cunning > top) { top = t.cunning; best = "cunning"; }
    if (!best) return "";
    auto it = p.names.epithets.find(best);
    return it == p.names.epithets.end() ? "" : it->second;
}

Person new_person(World& w, const Params& p, const std::string& race, const std::string& faction, const std::string& role) {
    Person person;
    int n = ++w.person_counters[race];
    person.id = race + std::to_string(n);
    person.race = race;
    person.faction = faction;
    person.role = role;
    person.born = w.year;
    const NameTable* t = table(p.names.people, race);
    if (t) {
        int ia = w.rng.range(0, static_cast<int>(t->a.size()));
        int ib = w.rng.range(0, static_cast<int>(t->b.size()));
        person.name = t->a[static_cast<size_t>(ia)] + t->b[static_cast<size_t>(ib)];
    } else person.name = person.id;
    person.traits.bold = w.rng.uniform();
    person.traits.pious = w.rng.uniform();
    person.traits.greedy = w.rng.uniform();
    person.traits.cunning = w.rng.uniform();
    std::string ep = epithet_for(p, person);
    if (!ep.empty()) person.name += " " + ep;
    return person;
}

void add_person(World& w, const Person& person) {
    w.people.push_back(person);
    std::sort(w.people.begin(), w.people.end(), [](const Person& a, const Person& b) { return a.id < b.id; });
}

static std::string cap(std::string s) {
    if (!s.empty()) s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

std::string display_name(const World& w, const std::string& id) {
    if (id.empty()) return "";
    if (id == "dragon") return "you";
    if (const Person* p = find_person(w, id)) return p->name;
    for (auto& d : w.dead_people) if (d.id == id) return d.name;
    if (const Region* r = find_region(w, id)) {
        if (!r->name.empty()) return r->name;
        std::string s = r->id;
        for (auto& ch : s) if (ch == '_') ch = ' ';
        return "the " + s;
    }
    return cap(id);
}

}

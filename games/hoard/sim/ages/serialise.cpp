#include <cstdio>
#include "ages.h"
#include "dragon_json.h"
#include "hook_json.h"
#include "genesis_json.h"
#include "json.h"
#include "names.h"
#include "oaths.h"
#include "offers.h"
#include "step.h"
#include "world_util.h"

namespace ages {

using nlohmann::json;

template <class T>
static void get(const json& j, const char* key, T& out) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) out = it->get<T>();
}

static json tags_json(const Tags& t) { return json(t); }
static Tags tags_from(const json& j, const char* key) {
    Tags t;
    auto it = j.find(key);
    if (it != j.end()) for (auto& v : *it) t.insert(v.get<std::string>());
    return t;
}

static json act_json(const Act& a) {
    json j;
    j["year"] = a.year;
    j["type"] = a.type;
    j["target"] = a.target;
    j["region"] = a.region;
    j["magnitude"] = a.magnitude;
    j["against"] = a.against;
    j["witnesses"] = a.witnesses;
    j["payload"] = a.payload;
    j["item"] = a.item;
    j["spell"] = a.spell;
    j["race"] = a.race;
    return j;
}

static Act act_from(const json& j) {
    Act a;
    get(j, "year", a.year);
    get(j, "type", a.type);
    get(j, "target", a.target);
    get(j, "region", a.region);
    get(j, "magnitude", a.magnitude);
    get(j, "against", a.against);
    get(j, "witnesses", a.witnesses);
    get(j, "payload", a.payload);
    get(j, "item", a.item);
    get(j, "spell", a.spell);
    get(j, "race", a.race);
    return a;
}

std::string acts_to_json(const std::vector<Act>& acts) {
    json arr = json::array();
    for (auto& a : acts) arr.push_back(act_json(a));
    return arr.dump(1);
}

bool acts_from_json(const std::string& text, std::vector<Act>& out, std::string& err) {
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded()) { err = "invalid act log json"; return false; }
    const json& arr = j.is_object() && j.contains("acts") ? j["acts"] : j;
    if (!arr.is_array()) { err = "act log must be an array"; return false; }
    for (auto& v : arr) out.push_back(act_from(v));
    return true;
}

static json memory_json(const MemoryEntry& m) {
    json j;
    j["year"] = m.year;
    j["type"] = m.type;
    j["magnitude"] = m.magnitude;
    j["weight"] = m.weight;
    j["fear"] = m.fear;
    j["faith"] = m.faith;
    j["tags"] = tags_json(m.tags);
    return j;
}

static json culture_json(const Culture& c) {
    json j;
    j["lifespan"] = c.lifespan;
    j["growth"] = c.growth;
    j["cohesion_decay"] = c.cohesion_decay;
    j["era_rate"] = c.era_rate;
    j["memory_halflife"] = c.memory_halflife;
    j["ruler"] = c.ruler_class;
    j["loot"] = c.loot;
    j["alliance"] = c.alliance;
    j["temperament"] = {{"war_appetite", c.temperament.war_appetite}, {"expansion", c.temperament.expansion},
                        {"home_terrain", c.temperament.home_terrain}, {"off_home_land", c.temperament.off_home_land},
                        {"settles_off_home", c.temperament.settles_off_home}};
    j["traits"] = {{"authority", c.traits.authority}, {"openness", c.traits.openness},
                   {"militarism", c.traits.militarism}, {"piety", c.traits.piety}};
    j["tags"] = tags_json(c.tags);
    return j;
}

static Culture culture_from(const json& j) {
    Culture c;
    get(j, "lifespan", c.lifespan);
    get(j, "growth", c.growth);
    get(j, "cohesion_decay", c.cohesion_decay);
    get(j, "era_rate", c.era_rate);
    get(j, "memory_halflife", c.memory_halflife);
    get(j, "ruler", c.ruler_class);
    get(j, "loot", c.loot);
    get(j, "alliance", c.alliance);
    if (j.contains("temperament")) {
        const json& t = j["temperament"];
        get(t, "war_appetite", c.temperament.war_appetite);
        get(t, "expansion", c.temperament.expansion);
        get(t, "home_terrain", c.temperament.home_terrain);
        get(t, "off_home_land", c.temperament.off_home_land);
        get(t, "settles_off_home", c.temperament.settles_off_home);
    }
    if (j.contains("traits")) {
        const json& t = j["traits"];
        get(t, "authority", c.traits.authority);
        get(t, "openness", c.traits.openness);
        get(t, "militarism", c.traits.militarism);
        get(t, "piety", c.traits.piety);
    }
    c.tags = tags_from(j, "tags");
    return c;
}

static json faction_json(const Faction& f) {
    json j;
    j["id"] = f.id;
    j["race"] = f.race;
    j["culture"] = culture_json(f.culture);
    j["pop"] = f.pop;
    j["regions"] = f.regions;
    j["wealth"] = f.wealth;
    j["cohesion"] = f.cohesion;
    j["era"] = f.era;
    j["era_progress"] = f.era_progress;
    j["fear"] = f.fear;
    j["faith"] = f.faith;
    j["memory"] = json::array();
    for (auto& m : f.memory) j["memory"].push_back(memory_json(m));
    j["ruler"] = {{"class", f.ruler.cls}, {"reigned", f.ruler.reigned}, {"reign_length", f.ruler.reign_length}};
    j["at_war_with"] = f.at_war_with;
    j["tags"] = tags_json(f.tags);
    j["anti_dragon"] = f.anti_dragon;
    j["born"] = f.born;
    j["last_region_lost"] = f.last_region_lost;
    j["last_invaded"] = f.last_invaded;
    j["last_war_year"] = f.last_war_year;
    j["last_growth"] = f.last_growth;
    j["slow_growth"] = f.slow_growth;
    j["max_regions"] = f.max_regions;
    j["max_realm"] = f.max_realm;
    j["era_years"] = f.era_years;
    j["eff"] = {{"growth", f.eff.growth}, {"cohesion_decay", f.eff.cohesion_decay}, {"era_rate", f.eff.era_rate}, {"war_chance", f.eff.war_chance}, {"settle_chance", f.eff.settle_chance}, {"anti_dragon_rate", f.eff.anti_dragon_rate}, {"diffusion", f.eff.diffusion}, {"faith_gain", f.eff.faith_gain}, {"succession_bonus", f.eff.succession_bonus}, {"vassal_wealth", f.eff.vassal_wealth}, {"memory_halflife", f.eff.memory_halflife}};
    return j;
}

static Faction faction_from(const json& j) {
    Faction f;
    get(j, "id", f.id);
    get(j, "race", f.race);
    if (j.contains("culture")) f.culture = culture_from(j["culture"]);
    get(j, "pop", f.pop);
    if (f.pop.empty()) f.pop[f.race] = 1.0;
    get(j, "regions", f.regions);
    get(j, "wealth", f.wealth);
    get(j, "cohesion", f.cohesion);
    get(j, "era", f.era);
    get(j, "era_progress", f.era_progress);
    get(j, "fear", f.fear);
    get(j, "faith", f.faith);
    if (j.contains("memory"))
        for (auto& m : j["memory"]) {
            MemoryEntry e;
            get(m, "year", e.year);
            get(m, "type", e.type);
            get(m, "magnitude", e.magnitude);
            get(m, "weight", e.weight);
            get(m, "fear", e.fear);
            get(m, "faith", e.faith);
            e.tags = tags_from(m, "tags");
            f.memory.push_back(e);
        }
    if (j.contains("ruler")) {
        get(j["ruler"], "class", f.ruler.cls);
        get(j["ruler"], "reigned", f.ruler.reigned);
        get(j["ruler"], "reign_length", f.ruler.reign_length);
    }
    get(j, "at_war_with", f.at_war_with);
    f.tags = tags_from(j, "tags");
    get(j, "anti_dragon", f.anti_dragon);
    get(j, "born", f.born);
    get(j, "last_region_lost", f.last_region_lost);
    get(j, "last_invaded", f.last_invaded);
    get(j, "last_war_year", f.last_war_year);
    get(j, "last_growth", f.last_growth);
    get(j, "slow_growth", f.slow_growth);
    get(j, "max_regions", f.max_regions);
    get(j, "max_realm", f.max_realm);
    get(j, "era_years", f.era_years);
    if (j.contains("eff")) {
        get(j["eff"], "growth", f.eff.growth);
        get(j["eff"], "cohesion_decay", f.eff.cohesion_decay);
        get(j["eff"], "era_rate", f.eff.era_rate);
        get(j["eff"], "war_chance", f.eff.war_chance);
        get(j["eff"], "settle_chance", f.eff.settle_chance);
        get(j["eff"], "anti_dragon_rate", f.eff.anti_dragon_rate);
        get(j["eff"], "diffusion", f.eff.diffusion);
        get(j["eff"], "faith_gain", f.eff.faith_gain);
        get(j["eff"], "succession_bonus", f.eff.succession_bonus);
        get(j["eff"], "vassal_wealth", f.eff.vassal_wealth);
        get(j["eff"], "memory_halflife", f.eff.memory_halflife);
    }
    return f;
}

static json region_json(const Region& r) {
    json j;
    j["id"] = r.id;
    j["neighbours"] = r.neighbours;
    j["owner"] = r.owner;
    j["terrain"] = r.terrain;
    j["land"] = r.land;
    j["eff_land"] = r.eff_land;
    j["burn"] = r.burn;
    j["lair"] = r.lair;
    j["tags"] = tags_json(r.tags);
    j["hot_decades"] = r.hot_decades;
    j["cold_decades"] = r.cold_decades;
    return j;
}

static Region region_from(const json& j) {
    Region r;
    get(j, "id", r.id);
    get(j, "neighbours", r.neighbours);
    get(j, "owner", r.owner);
    get(j, "terrain", r.terrain);
    get(j, "land", r.land);
    get(j, "burn", r.burn);
    get(j, "lair", r.lair);
    r.tags = tags_from(j, "tags");
    get(j, "hot_decades", r.hot_decades);
    get(j, "cold_decades", r.cold_decades);
    r.eff_land = r.land;
    get(j, "eff_land", r.eff_land);
    return r;
}

static json oath_json(const Oath& o) {
    json j;
    j["id"] = o.id;
    j["parties"] = {o.parties[0], o.parties[1]};
    j["sworn"] = o.sworn;
    j["expires"] = o.expires;
    j["clauses"] = json::array();
    for (auto& c : o.clauses) j["clauses"].push_back({{"who", c.who}, {"pred", c.pred}, {"params", c.params}});
    j["stakes"] = {{"breach_by_dragon", o.breach_by_dragon}, {"breach_by_them", o.breach_by_them}};
    j["status"] = o.status;
    if (!o.kind.empty()) {
        j["kind"] = o.kind;
        j["ask"] = o.ask;
        j["target"] = o.target;
        j["motive"] = o.motive;
        j["envoy"] = o.envoy;
        j["is_lie"] = o.is_lie;
        j["fulfilled"] = o.fulfilled;
        j["price"] = o.price;
    }
    return j;
}

static Oath oath_from(const json& j) {
    Oath o;
    get(j, "id", o.id);
    if (j.contains("parties") && j["parties"].size() == 2) {
        o.parties[0] = j["parties"][0].get<std::string>();
        o.parties[1] = j["parties"][1].get<std::string>();
    }
    get(j, "sworn", o.sworn);
    get(j, "expires", o.expires);
    if (j.contains("clauses"))
        for (auto& c : j["clauses"]) {
            Clause cl;
            get(c, "who", cl.who);
            get(c, "pred", cl.pred);
            get(c, "params", cl.params);
            o.clauses.push_back(cl);
        }
    if (j.contains("stakes")) {
        get(j["stakes"], "breach_by_dragon", o.breach_by_dragon);
        get(j["stakes"], "breach_by_them", o.breach_by_them);
    }
    get(j, "status", o.status);
    get(j, "kind", o.kind);
    get(j, "ask", o.ask);
    get(j, "target", o.target);
    get(j, "motive", o.motive);
    get(j, "envoy", o.envoy);
    get(j, "is_lie", o.is_lie);
    get(j, "fulfilled", o.fulfilled);
    get(j, "price", o.price);
    return o;
}

static json modifier_json(const Modifier& m) {
    return {{"source", m.source}, {"target", m.target}, {"var", m.var}, {"op", m.op},
            {"value", m.value}, {"from", m.from}, {"until", m.until}};
}

static Modifier modifier_from(const json& j) {
    Modifier m;
    get(j, "source", m.source);
    get(j, "target", m.target);
    get(j, "var", m.var);
    get(j, "op", m.op);
    get(j, "value", m.value);
    get(j, "from", m.from);
    get(j, "until", m.until);
    return m;
}

static json event_json(const Event& e) {
    return {{"id", e.id}, {"year", e.year}, {"kind", e.kind}, {"subject", e.subject}, {"object", e.object},
            {"magnitude", e.magnitude}, {"text_key", e.text_key}, {"tags", tags_json(e.tags)}, {"count", e.count},
            {"cause", {{"refs", e.cause.refs}, {"text_key", e.cause.text_key}, {"params", e.cause.params}}}, {"thread", e.thread},
            {"importance", e.importance}};
}

static Event event_from(const json& j) {
    Event e;
    get(j, "id", e.id);
    get(j, "year", e.year);
    get(j, "kind", e.kind);
    get(j, "subject", e.subject);
    get(j, "object", e.object);
    get(j, "magnitude", e.magnitude);
    get(j, "text_key", e.text_key);
    e.tags = tags_from(j, "tags");
    get(j, "count", e.count);
    get(j, "thread", e.thread);
    get(j, "importance", e.importance);
    if (j.contains("cause")) {
        get(j["cause"], "refs", e.cause.refs);
        get(j["cause"], "text_key", e.cause.text_key);
        get(j["cause"], "params", e.cause.params);
    }
    return e;
}

static json person_json(const Person& x) {
    return {{"id", x.id}, {"name", x.name}, {"race", x.race}, {"faction", x.faction}, {"role", x.role}, {"born", x.born},
            {"dies", x.dies}, {"traits", {{"bold", x.traits.bold}, {"pious", x.traits.pious}, {"greedy", x.traits.greedy}, {"cunning", x.traits.cunning}}},
            {"tags", tags_json(x.tags)}, {"memory_of_dragon", x.memory_of_dragon}, {"hostage_since", x.hostage_since}};
}

static Person person_from(const json& j) {
    Person x;
    get(j, "id", x.id);
    get(j, "name", x.name);
    get(j, "race", x.race);
    get(j, "faction", x.faction);
    get(j, "role", x.role);
    get(j, "born", x.born);
    get(j, "dies", x.dies);
    if (j.contains("traits")) {
        get(j["traits"], "bold", x.traits.bold);
        get(j["traits"], "pious", x.traits.pious);
        get(j["traits"], "greedy", x.traits.greedy);
        get(j["traits"], "cunning", x.traits.cunning);
    }
    x.tags = tags_from(j, "tags");
    get(j, "memory_of_dragon", x.memory_of_dragon);
    get(j, "hostage_since", x.hostage_since);
    return x;
}

static json offer_json(const World& w, const Params& p, const Offer& of) {
    json j = {{"id", of.id}, {"text_key", of.text_key}, {"kind", of.kind}, {"from_faction", of.from_faction}, {"envoy", of.envoy},
              {"envoy_name", display_name(w, of.envoy)}, {"target", of.target}, {"target_name", display_name(w, of.target)},
              {"stated_reason", of.stated_reason}, {"expires", of.expires}, {"price", of.price}, {"marked", of.marked},
              {"ask", of.draft.ask}, {"oath", oath_json(of.draft)}};
    if (w.dragon.knowledge >= p.c.offer_truth_knowledge) { j["eyes_say_otherwise"] = of.is_lie; j["motive"] = of.motive; }
    return j;
}

static Offer offer_from(const json& j) {
    Offer of;
    get(j, "id", of.id);
    get(j, "text_key", of.text_key);
    get(j, "kind", of.kind);
    get(j, "from_faction", of.from_faction);
    get(j, "envoy", of.envoy);
    get(j, "target", of.target);
    get(j, "stated_reason", of.stated_reason);
    get(j, "motive", of.motive);
    get(j, "is_lie", of.is_lie);
    get(j, "marked", of.marked);
    get(j, "expires", of.expires);
    get(j, "price", of.price);
    if (j.contains("oath")) of.draft = oath_from(j["oath"]);
    return of;
}

static json record_json(const FactionRecord& r) {
    return {{"id", r.id}, {"race", r.race}, {"born", r.born}, {"died", r.died},
            {"max_regions", r.max_regions}, {"max_realm", r.max_realm}, {"era_years", r.era_years}};
}

static json world_json(const World& w) {
    json j;
    j["year"] = w.year;
    j["seed"] = w.seed;
    j["rng"] = {{"state", w.rng.state}, {"inc", w.rng.inc}};
    j["magic"] = w.magic;
    j["regions"] = json::array();
    for (auto& r : w.regions) j["regions"].push_back(region_json(r));
    j["factions"] = json::array();
    for (auto& f : w.factions) j["factions"].push_back(faction_json(f));
    const Dragon& d = w.dragon;
    j["dragon"] = {
        {"size", d.size}, {"knowledge", d.knowledge}, {"magic", d.magic}, {"faith_pool", d.faith_pool}, {"hunger", d.hunger},
        {"hoard", {{"gold", d.hoard.gold}, {"books", d.hoard.books}, {"artifacts", d.hoard.artifacts},
                   {"art", d.hoard.art}, {"royals", d.hoard.royals}, {"food", d.hoard.food}}},
        {"lair", {{"depth", d.lair_depth}, {"library", d.lair_library}, {"sky", d.lair_sky}}},
        {"lineages", d.lineages}, {"royals_digested", d.royals_digested}, {"tags", tags_json(d.tags)},
        {"phase", d.phase}, {"hostages", d.hostages}, {"known_people", json::array()}};
    for (auto& k : d.known_people)
        j["dragon"]["known_people"].push_back({{"id", k.id}, {"relationship", k.relationship}, {"met", k.met}, {"how", k.how}, {"tags", tags_json(k.tags)}});
    dragon_state_to_json(d, j["dragon"]);   // v0.4 (S1)
    j["oaths"] = json::array();
    for (auto& o : w.oaths) j["oaths"].push_back(oath_json(o));
    j["modifiers"] = json::array();
    for (auto& m : w.modifiers) j["modifiers"].push_back(modifier_json(m));
    j["acts"] = json::array();
    for (auto& a : w.acts) j["acts"].push_back(act_json(a));
    j["chronicle"] = json::array();
    for (auto& e : w.chronicle) j["chronicle"].push_back(event_json(e));
    j["tags"] = tags_json(w.tags);
    j["wars"] = json::array();
    for (auto& x : w.wars)
        j["wars"].push_back({{"a", x.a}, {"b", x.b}, {"started", x.started}, {"decades", x.decades},
                             {"taken_from_a", x.taken_from_a}, {"taken_from_b", x.taken_from_b},
                             {"allies_a", x.allies_a}, {"allies_b", x.allies_b}, {"coalition", x.coalition},
                             {"years_fought", x.years_fought}, {"declared", x.declared}, {"thread", x.thread}});
    j["remnants"] = json::array();
    for (auto& r : w.remnants)
        j["remnants"].push_back({{"race", r.race}, {"strength", r.strength}, {"origin", r.origin}, {"max_era", r.max_era}});
    j["race_max_era"] = w.race_max_era;
    j["race_last_region"] = w.race_last_region;
    j["human_share_decades"] = w.human_share_decades;
    j["graveyard"] = json::array();
    for (auto& g : w.graveyard) j["graveyard"].push_back(record_json(g));
    j["next_serial"] = w.next_serial;
    j["next_event_id"] = w.next_event_id;
    j["next_offer_serial"] = w.next_offer_serial;
    j["slow_clock"] = w.slow_clock;
    j["people"] = json::array();
    for (auto& x : w.people) j["people"].push_back(person_json(x));
    j["dead_people"] = json::array();
    for (auto& x : w.dead_people)
        j["dead_people"].push_back({{"id", x.id}, {"name", x.name}, {"race", x.race}, {"faction", x.faction}, {"role", x.role}, {"born", x.born}, {"died", x.died}});
    j["person_counters"] = w.person_counters;
    j["offers_pending"] = json::array();
    for (auto& of : w.offers) {
        json o = {{"id", of.id}, {"text_key", of.text_key}, {"kind", of.kind}, {"from_faction", of.from_faction}, {"envoy", of.envoy}, {"target", of.target},
                  {"motive", of.motive}, {"stated_reason", of.stated_reason}, {"is_lie", of.is_lie}, {"marked", of.marked}, {"expires", of.expires}, {"price", of.price}, {"oath", oath_json(of.draft)}};
        j["offers_pending"].push_back(o);
    }
    j["sleep_started"] = w.sleep_started;
    j["threads_open"] = w.threads_open;
    j["timed_tags"] = json::array();
    for (auto& t : w.timed_tags)
        j["timed_tags"].push_back({{"on", t.on}, {"target", t.target}, {"tag", t.tag}, {"from", t.from}, {"until", t.until}, {"event", t.event}, {"inherit", t.inherit}});
    j["card_fired"] = w.card_fired;
    genesis_fields_to_json(w, j);   // v0.4 world (S2)
    return j;
}

// v0.4 hooks (S3): the saved and the viewed hook shapes are the same; the params are needed for the fight damage
static json world_json(const World& w, const Params& p) {
    json j = world_json(w);
    hooks_state_to_json(w, p, j);
    return j;
}

std::string world_to_json(const World& w) { Params none; return world_json(w, none).dump(1); }
json world_state_json(const World& w) { Params none; return world_json(w, none); }

static const char* const era_names[] = {"bronze", "iron", "castles", "gunpowder", "arcane"};

static json offers_json(const World& w, const Params& p) {
    json arr = json::array();
    for (auto& of : w.offers) arr.push_back(offer_json(w, p, of));
    return arr;
}

static json phase_json(const World& w, const Params& p) {
    const Dragon& d = w.dragon;
    return {{"phase", d.phase}, {"short_sleep", short_sleep(d, p.c)}, {"long_sleep", long_sleep(d, p.c)},
            {"ready_to_moult", ready_to_moult(d, p.c)}, {"next_phase_score", phase_score(d, p.c, d.phase + 1)}, {"stiff", has_tag(d.tags, "stiff")}};
}

std::string people_json(const World& w, const Params& p) {
    (void)p;
    json arr = json::array();
    for (auto& x : w.people) {
        json j = person_json(x);
        if (const Known* k = find_known(w, x.id)) j["known"] = {{"relationship", k->relationship}, {"met", k->met}, {"how", k->how}, {"tags", tags_json(k->tags)}};
        arr.push_back(j);
    }
    return json{{"ok", true}, {"people", arr}}.dump();
}

// a region's share of its owner's productive land: the settlement size the shell draws
static double region_yield(const Region& r) { return r.eff_land * (1.0 - r.burn); }

static std::string capital_of(const World& w, const Faction& f) {
    std::string best;
    double top = -1;
    for (auto& rid : f.regions)
        if (const Region* r = find_region(w, rid))
            if (region_yield(*r) > top) { top = region_yield(*r); best = rid; }
    return best;
}

// one entry per (attacker member, defender member) pair with a shared border, placed on the defender's border region
static json armies_json(const World& w) {
    json arr = json::array();
    auto members = [](const War& x, bool a) { std::vector<std::string> m = a ? x.allies_a : x.allies_b; m.insert(m.begin(), a ? x.a : x.b); return m; };
    for (auto& x : w.wars) {
        for (int side = 0; side < 2; ++side) {
            for (auto& att : members(x, side == 0)) {
                const Faction* fa = find_faction(w, att);
                if (!fa) continue;
                for (auto& def : members(x, side != 0)) {
                    const Faction* fd = find_faction(w, def);
                    if (!fd) continue;
                    std::string region;
                    for (auto& rid : fd->regions) {
                        const Region* r = find_region(w, rid);
                        if (!r) continue;
                        for (auto& n : r->neighbours) if (contains(fa->regions, n)) { region = rid; break; }
                        if (!region.empty()) break;
                    }
                    if (region.empty()) continue;
                    arr.push_back({{"attacker", att}, {"defender", def}, {"region", region}, {"coalition", x.coalition},
                                   {"war", x.a + "~" + x.b}});
                }
            }
        }
    }
    return arr;
}

static json spells_json(const Params& p) {
    json arr = json::array();
    for (auto& kv : p.spells) {
        const SpellDef& s = kv.second;
        json j = {{"id", s.id}, {"enabled", s.enabled}, {"gate", s.gate}, {"target", s.target},
                  {"gate_min", std::max(s.gate_min, s.cost)}, {"cost", s.cost}};
        if (s.modifier.present) j["modifier"] = {{"var", s.modifier.var}, {"op", s.modifier.op}, {"value", s.modifier.value}, {"decades", s.modifier.decades}};
        if (!s.spawn_race.empty()) j["spawn"] = s.spawn_race;
        arr.push_back(j);
    }
    json t = {{"gate_size", p.terraform.gate_size}, {"cost_size_fraction", p.terraform.cost_size_fraction}, {"kinds", json::array()}};
    for (auto& kv : p.terraform.kinds) t["kinds"].push_back({{"kind", kv.first}, {"tag", kv.second.tag}});
    json d = {{"gate_knowledge", p.c.demand_knowledge}, {"fear", p.c.demand_fear}, {"tribute_rate", p.c.demand_tribute_rate},
              {"preds", json::array({"tribute", "no_walls", "no_settle"})}};
    return {{"spells", arr}, {"terraform", t}, {"demand", d}};
}

std::string spells_catalogue_json(const Params& p) { return spells_json(p).dump(); }

std::string world_view_json(const World& w, const Params& p) {
    json j = world_json(w, p);   // v0.4 hooks (S3): hooks in the view shape
    std::map<std::string, std::string> capitals;
    for (size_t i = 0; i < w.factions.size(); ++i) {
        const Faction& f = w.factions[i];
        json& fj = j["factions"][i];
        fj["power"] = power(f, p.c);
        fj["era_name"] = era_names[f.era < 0 ? 0 : (f.era > 4 ? 4 : f.era)];
        fj["dominant_race"] = dominant_race(f);
        fj["overlord"] = overlord_of(f);
        json vassals = json::array();
        for (auto& g : w.factions) if (overlord_of(g) == f.id) vassals.push_back(g.id);
        fj["vassals"] = vassals;
        capitals[f.id] = capital_of(w, f);
        fj["capital"] = capitals[f.id];
        json allies = json::array();
        for (auto& g : w.factions) if (g.id != f.id && allied(f, g)) allies.push_back(g.id);
        fj["allies"] = allies;
        json coalitions = json::array();
        for (auto& x : w.wars)
            if (x.coalition && (x.a == f.id || contains(x.allies_a, f.id))) coalitions.push_back(x.b);
        fj["coalition_against"] = coalitions;
        fj["hostage_taken"] = has_tag(f.tags, "hostage_taken");
        fj["altar"] = f.faith > p.c.reaction_faith_high;
        fj["loot"] = f.culture.loot;
        auto rt = p.races.find(dominant_race(f));
        if (rt != p.races.end()) fj["loot"] = rt->second.culture.loot;
    }
    for (size_t i = 0; i < w.regions.size(); ++i) {
        const Region& r = w.regions[i];
        json& rj = j["regions"][i];
        double share = 0;
        if (const Faction* o = find_faction(w, r.owner)) {
            double total = 0;
            for (auto& rid : o->regions) if (const Region* rr = find_region(w, rid)) total += region_yield(*rr);
            share = total > 0 ? region_yield(r) / total : 1.0 / std::max<size_t>(1, o->regions.size());
        }
        rj["wealth_share"] = share;
        rj["eff_land"] = r.eff_land;
        rj["capital"] = !r.owner.empty() && capitals[r.owner] == r.id;
    }
    j["armies"] = armies_json(w);
    j["dragon"]["threat"] = threat(w.dragon, p.c);
    j["dragon"]["hunger_need"] = hunger_need(w, p.c);
    j["dragon"]["known_names"] = json::object();
    for (auto& k : w.dragon.known_people) j["dragon"]["known_names"][k.id] = display_name(w, k.id);
    j["phase"] = phase_json(w, p);
    j["offers"] = offers_json(w, p);
    j["catalogue"] = spells_json(p);
    j["wake_reason"] = w.wake_reason;
    dragon_view_to_json(w, p, j);   // v0.4 (S1)
    return j.dump();
}

std::string cast_result_to_json(const CastResult& r) {
    json j = {{"ok", true}, {"cast", r.cast}, {"spell", r.spell}, {"target", r.target}, {"region", r.region}, {"reason", r.reason},
              {"spawned_faction", r.spawned_faction}, {"war_with", r.war_with}};
    if (r.has_modifier) j["modifier"] = modifier_json(r.modifier);
    return j.dump();
}

std::string demand_result_to_json(const DemandResult& r) {
    json j = {{"ok", true}, {"sworn", r.sworn}, {"target", r.target}, {"pred", r.pred}, {"reason", r.reason}};
    if (r.sworn) j["oath"] = oath_json(r.oath);
    return j.dump();
}

std::string terraform_result_to_json(const TerraformResult& r) {
    json j = {{"ok", true}, {"done", r.done}, {"tag", r.tag}, {"reason", r.reason}, {"cost", r.cost}};
    if (r.has_modifier) j["modifier"] = modifier_json(r.modifier);
    return j.dump();
}

std::string forecast_to_json(const Forecast& f) {
    json j;
    j["years"] = f.years;
    j["wake_reason"] = f.wake_reason;
    j["size"] = f.size;
    j["knowledge"] = f.knowledge;
    j["magic"] = f.magic;
    j["hunger"] = f.hunger;
    j["strongest"] = f.strongest;
    j["strongest_era"] = f.strongest_era;
    j["slay_possible"] = f.slay_possible;
    forecast_hook_to_json(f, j);   // v0.4 hooks (S3)
    return j.dump();
}

std::string sleep_result_to_json(const SleepResult& r, const World& w, const Params& p) {
    json j;
    j["wake_reason"] = r.wake_reason;
    j["years_slept"] = r.years_slept;
    j["year"] = w.year;
    j["chronicle"] = json::array();
    for (auto& e : r.chronicle) j["chronicle"].push_back(event_json(e));
    j["offers"] = offers_json(w, p);
    sleep_growth_to_json(r, j);   // v0.4 (S1)
    j["hooks"] = hooks_json(w, p, r.hooks);   // v0.4 hooks (S3)
    return j.dump();
}

std::string chronicle_hash_hex(const World& w) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(chronicle_hash(w)));
    return buf;
}

bool world_from_json(const std::string& text, World& w, std::string& err) {
    json j = json::parse(text, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { err = "invalid world json"; return false; }
    get(j, "year", w.year);
    get(j, "seed", w.seed);
    if (j.contains("rng")) { get(j["rng"], "state", w.rng.state); get(j["rng"], "inc", w.rng.inc); }
    get(j, "magic", w.magic);
    if (j.contains("regions")) for (auto& v : j["regions"]) w.regions.push_back(region_from(v));
    if (j.contains("factions")) for (auto& v : j["factions"]) w.factions.push_back(faction_from(v));
    if (j.contains("dragon")) {
        const json& d = j["dragon"];
        get(d, "size", w.dragon.size);
        get(d, "knowledge", w.dragon.knowledge);
        get(d, "magic", w.dragon.magic);
        get(d, "faith_pool", w.dragon.faith_pool);
        get(d, "hunger", w.dragon.hunger);
        if (d.contains("hoard")) {
            get(d["hoard"], "gold", w.dragon.hoard.gold);
            get(d["hoard"], "books", w.dragon.hoard.books);
            get(d["hoard"], "artifacts", w.dragon.hoard.artifacts);
            get(d["hoard"], "art", w.dragon.hoard.art);
            get(d["hoard"], "royals", w.dragon.hoard.royals);
            get(d["hoard"], "food", w.dragon.hoard.food);
        }
        if (d.contains("lair")) {
            get(d["lair"], "depth", w.dragon.lair_depth);
            get(d["lair"], "library", w.dragon.lair_library);
            get(d["lair"], "sky", w.dragon.lair_sky);
        }
        get(d, "lineages", w.dragon.lineages);
        get(d, "royals_digested", w.dragon.royals_digested);
        w.dragon.tags = tags_from(d, "tags");
        get(d, "phase", w.dragon.phase);
        get(d, "hostages", w.dragon.hostages);
        if (d.contains("known_people"))
            for (auto& v : d["known_people"]) {
                Known k;
                get(v, "id", k.id);
                get(v, "relationship", k.relationship);
                get(v, "met", k.met);
                get(v, "how", k.how);
                k.tags = tags_from(v, "tags");
                w.dragon.known_people.push_back(k);
            }
        dragon_state_from_json(d, w.dragon);   // v0.4 (S1)
    }
    if (j.contains("oaths")) for (auto& v : j["oaths"]) w.oaths.push_back(oath_from(v));
    if (j.contains("modifiers")) for (auto& v : j["modifiers"]) w.modifiers.push_back(modifier_from(v));
    if (j.contains("acts")) for (auto& v : j["acts"]) w.acts.push_back(act_from(v));
    if (j.contains("chronicle")) for (auto& v : j["chronicle"]) w.chronicle.push_back(event_from(v));
    w.tags = tags_from(j, "tags");
    if (j.contains("wars"))
        for (auto& v : j["wars"]) {
            War x;
            get(v, "a", x.a);
            get(v, "b", x.b);
            get(v, "started", x.started);
            get(v, "decades", x.decades);
            get(v, "taken_from_a", x.taken_from_a);
            get(v, "taken_from_b", x.taken_from_b);
            get(v, "allies_a", x.allies_a);
            get(v, "allies_b", x.allies_b);
            get(v, "coalition", x.coalition);
            get(v, "years_fought", x.years_fought);
            get(v, "declared", x.declared);
            get(v, "thread", x.thread);
            w.wars.push_back(x);
        }
    if (j.contains("remnants"))
        for (auto& v : j["remnants"]) {
            Remnant r;
            get(v, "race", r.race);
            get(v, "strength", r.strength);
            get(v, "origin", r.origin);
            get(v, "max_era", r.max_era);
            w.remnants.push_back(r);
        }
    get(j, "race_max_era", w.race_max_era);
    get(j, "race_last_region", w.race_last_region);
    get(j, "human_share_decades", w.human_share_decades);
    get(j, "next_offer_serial", w.next_offer_serial);
    if (j.contains("graveyard"))
        for (auto& v : j["graveyard"]) {
            FactionRecord r;
            get(v, "id", r.id);
            get(v, "race", r.race);
            get(v, "born", r.born);
            get(v, "died", r.died);
            get(v, "max_regions", r.max_regions);
            get(v, "max_realm", r.max_realm);
            get(v, "era_years", r.era_years);
            w.graveyard.push_back(r);
        }
    get(j, "next_serial", w.next_serial);
    get(j, "next_event_id", w.next_event_id);
    get(j, "slow_clock", w.slow_clock);
    if (j.contains("people")) for (auto& v : j["people"]) w.people.push_back(person_from(v));
    if (j.contains("dead_people"))
        for (auto& v : j["dead_people"]) {
            DeadPerson x;
            get(v, "id", x.id);
            get(v, "name", x.name);
            get(v, "race", x.race);
            get(v, "faction", x.faction);
            get(v, "role", x.role);
            get(v, "born", x.born);
            get(v, "died", x.died);
            w.dead_people.push_back(x);
        }
    get(j, "person_counters", w.person_counters);
    if (j.contains("offers_pending")) for (auto& v : j["offers_pending"]) w.offers.push_back(offer_from(v));
    get(j, "sleep_started", w.sleep_started);
    get(j, "threads_open", w.threads_open);
    if (j.contains("timed_tags"))
        for (auto& v : j["timed_tags"]) {
            TimedTag t;
            get(v, "on", t.on);
            get(v, "target", t.target);
            get(v, "tag", t.tag);
            get(v, "from", t.from);
            get(v, "until", t.until);
            get(v, "event", t.event);
            get(v, "inherit", t.inherit);
            w.timed_tags.push_back(t);
        }
    get(j, "card_fired", w.card_fired);
    hooks_state_from_json(j, w);   // v0.4 hooks (S3)
    genesis_fields_from_json(j, w);   // v0.4 world (S2)
    sort_by_id(w);
    return true;
}

}

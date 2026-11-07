#include <algorithm>
#include "report.h"
#include "ages/oaths.h"
#include "ages/world_util.h"

// item 15: facts to acts (raid-design.md section 3). Pure arithmetic over the report and the sortie snapshot.
namespace ages {

namespace {

enum Stage { st_take, st_hostage, st_kill, st_burn, st_civilians, st_defend, st_show, st_receive, st_fight, st_spells, st_speech, st_wound };

struct Slot {
    int stage;
    std::string key;
    Act act;
};

struct Diff {
    const World& w;
    const Params& p;
    const RaidDiffBook& book;
    const SortieState& snap;
    std::vector<Slot> slots;
    std::vector<std::string> warnings;
    std::map<std::string, int> shown;   // region -> strongest presence rank (1 flyover, 2 roar, 3 land)
    std::vector<std::string> shown_order;

    Diff(const World& w_, const Params& p_) : w(w_), p(p_), book(p_.raid_diff), snap(w_.sortie) {}

    void warn(const std::string& code) { warnings.push_back(code); }

    void add(int stage, const std::string& key, const Act& a) {
        if (!(a.magnitude > 0) && a.type != "receive" && a.type != "fight" && a.type != "take_hostage" && a.type != "spell" && stage < st_spells) return;
        for (auto& s : slots) if (s.key == key) { s.act.magnitude += a.magnitude; return; }
        slots.push_back({stage, key, a});
    }

    bool acted_at(const std::string& region) const {
        for (auto& s : slots) if (s.stage < st_spells && s.act.region == region && s.stage != st_show) return true;
        return false;
    }

    const SortieSettlement* settlement(const std::string& region) const {
        for (auto& s : snap.settlements) if (s.region == region) return &s;
        return nullptr;
    }

    const SortieCourt* court(const std::string& id) const {
        for (auto& c : snap.courts) if (c.id == id) return &c;
        return nullptr;
    }

    static Act act(const char* type, const std::string& target, const std::string& region, double m) {
        Act a;
        a.type = type;
        a.target = target;
        a.region = region;
        a.magnitude = m;
        return a;
    }

    static double share(double part, double whole) { return whole > 0 ? part / whole : 0; }

    void burns(const RaidSettlement& x, const SortieSettlement& s) {
        double plain = 0;
        for (auto& kv : x.destroyed) {
            if (!(kv.second > 0)) continue;
            const std::string& cls = kv.first;
            if (contains(book.plain, cls)) { plain += kv.second; continue; }
            if (!contains(book.structures, cls)) { warn("raid.unknown_class|class=" + cls); continue; }
            auto held = s.structures.find(cls);
            if (held == s.structures.end() || !(held->second > 0)) { warn("raid.no_structure|region=" + s.region + "|structure=" + cls); continue; }
            if (kv.second < book.structure_threshold * held->second) { plain += kv.second; continue; }
            Act a = act("burn", s.owner, s.region, share(kv.second, s.value));
            a.item = cls;
            add(st_burn, "burn|" + s.region + "|" + cls, a);
        }
        if (plain > 0 && !(s.value > 0)) warn("raid.no_value|region=" + s.region);
        add(st_burn, "burn|" + s.region + "|", act("burn", s.owner, s.region, share(plain, s.value)));
    }

    void kills(const RaidSettlement& x, const SortieSettlement& s) {
        if (x.soldiers > 0) {
            Act a = act("kill", s.owner, s.region, share(x.soldiers, s.soldiers));
            a.payload["soldiers"] = 1;
            add(st_kill, "kill|" + s.region + "|army|" + s.owner, a);
        }
        if (x.garrison > 0) {
            Act a = act("kill", s.owner, s.region, share(x.garrison, s.garrison));
            a.payload["soldiers"] = 1;
            a.args["who"] = "garrison";
            add(st_kill, "kill|" + s.region + "|garrison", a);
        }
        for (auto& id : x.persons) {
            Act a = act("kill", id, s.region, book.kill_person);
            a.target_kind = "person";
            add(st_kill, "kill|person|" + id, a);
        }
        if (x.civilians > 0) {
            Act a = act("kill", s.owner, s.region, share(x.civilians, s.population));
            a.args["who"] = "civilians";
            add(st_civilians, "kill|" + s.region + "|civilians", a);
        }
    }

    void loot(const RaidSettlement& x, const SortieSettlement& s) {
        const SortieCourt* c = court(s.owner);
        for (auto& kv : x.collected) {
            if (!(kv.second > 0)) continue;
            double held = 0;
            if (c) { auto it = c->holds.find(kv.first); if (it != c->holds.end()) held = it->second; else { warn("raid.unknown_class|class=" + kv.first); continue; } }
            if (!(held > 0)) { warn("raid.no_holdings|court=" + s.owner + "|what=" + kv.first); continue; }
            Act a = act("take", s.owner, s.region, share(kv.second, held));
            a.payload[kv.first] = 1;
            add(st_take, "take|" + s.region + "|" + kv.first, a);
        }
        for (auto& id : x.items) {
            Act a = act("take", s.owner, s.region, book.item);
            a.item = id;
            a.payload["artifacts"] = 1;
            add(st_take, "take|" + s.region + "|item|" + id, a);
        }
        size_t n = 0;
        for (auto& id : x.taken) {
            Act a = act("take_hostage", s.owner, s.region, book.hostage);
            a.item = id;
            add(st_hostage, "take_hostage|" + s.region + "|" + std::to_string(n++), a);
        }
    }

    void presence(const RaidSettlement& x, const SortieSettlement& s) {
        int rank = x.landed_in_square ? 3 : x.roared ? 2 : x.seconds_below_150 >= book.flyover_seconds && !acted_at(s.region) ? 1 : 0;
        if (!rank) return;
        if (!shown.count(s.region)) shown_order.push_back(s.region);
        shown[s.region] = std::max(shown[s.region], rank);
    }

    void shows() {
        for (auto& region : shown_order) {
            int rank = shown[region];
            if (rank == 1 && acted_at(region)) continue;
            const SortieSettlement* s = settlement(region);
            Act a = act("show", s->owner, region, rank == 3 ? book.show_land : rank == 2 ? book.show_roar : book.show_flyover);
            a.args["kind"] = rank == 3 ? "land" : rank == 2 ? "roar" : "flyover";
            add(st_show, "show|" + region, a);
        }
    }

    void armies(const RaidReport& r) {
        for (auto& x : r.armies) {
            const SortieSettlement* at = settlement(x.region);
            std::string defender = x.defender.empty() && at ? at->owner : x.defender;
            if (!find_faction(w, x.attacker) || x.region.empty()) { warn("raid.unknown_army|attacker=" + x.attacker); continue; }
            if (x.killed_share > 0) {
                Act a = act("kill", x.attacker, x.region, x.killed_share);
                a.payload["soldiers"] = 1;
                add(st_kill, "kill|" + x.region + "|army|" + x.attacker, a);
            }
            const Faction* fd = find_faction(w, defender);
            bool oath = fd && (active_oath_with_pred(w, defender, "defend") || active_oath_with_pred(w, defender, "no_harm"));
            if (!(x.repelled || (x.killed_share > 0 && oath))) continue;
            if (!fd || !at_war(*fd, x.attacker)) { warn("raid.not_at_war|attacker=" + x.attacker + "|defender=" + defender); continue; }
            Act a = act("defend", defender, x.region, x.killed_share > 0 ? x.killed_share : book.repelled);
            a.against = x.attacker;
            add(st_defend, "defend|" + x.region + "|" + x.attacker, a);
        }
    }

    void lair(const RaidReport& r) {
        for (auto& x : r.lair.received) {
            Act a = act("receive", "", "", 0);
            a.target_kind = "lair";
            a.args["who"] = x.id;
            a.args["outcome"] = x.outcome;
            add(st_receive, "receive|" + x.id, a);
        }
        for (auto& id : r.lair.fought) {
            Act a = act("fight", id, "", 0);
            a.target_kind = "person";
            add(st_fight, "fight|" + id, a);
        }
    }

    void passthrough(const RaidReport& r) {
        size_t n = 0;
        for (auto& a : r.spells) add(st_spells, "spells|" + std::to_string(n++), a);
        for (auto& a : r.speech) add(st_speech, "speech|" + std::to_string(n++), a);
        for (auto& kv : r.wounds) {
            if (!(kv.second > 0)) continue;
            size_t bar = kv.first.find('|');
            std::string region = bar == std::string::npos ? "" : kv.first.substr(0, bar), item = bar == std::string::npos ? kv.first : kv.first.substr(bar + 1);
            const Region* reg = find_region(w, region);
            Act a = act("wound", reg ? reg->owner : "", region, kv.second);
            a.item = item;
            add(st_wound, "wound|" + kv.first, a);
        }
    }

    DiffResult run(const RaidReport& r) {
        for (auto& x : r.settlements) {
            const SortieSettlement* s = settlement(x.region);
            if (!s) { warn("raid.unknown_region|region=" + x.region); continue; }
            loot(x, *s);
            kills(x, *s);
            burns(x, *s);
            presence(x, *s);
        }
        armies(r);
        shows();
        lair(r);
        passthrough(r);
        std::stable_sort(slots.begin(), slots.end(), [](const Slot& a, const Slot& b) { return a.stage < b.stage; });
        DiffResult out;
        for (auto& s : slots) {
            Act a = s.act;
            if (s.stage < st_receive && a.magnitude > 1) { warn("raid.capped|type=" + a.type + "|region=" + a.region); a.magnitude = 1; }
            out.acts.push_back(a);
        }
        out.warnings = warnings;
        return out;
    }
};

}

DiffResult raid_diff(const World& w, const Params& p, const RaidReport& r) { return Diff(w, p).run(r); }

}

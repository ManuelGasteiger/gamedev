#include "act_gates.h"
#include <algorithm>
#include <cstdio>
#include "talents.h"
#include "world_util.h"

namespace ages {

namespace {

std::string num(double v) { char b[32]; std::snprintf(b, sizeof b, "%g", v); return b; }

double pool_of(const Dragon& d, const std::string& k) {
    if (k == "magic") return d.magic;
    if (k == "knowledge") return d.knowledge;
    if (k == "faith_pool") return d.faith_pool;
    return 0;
}

std::string alt_refusal(const Dragon& d, const GateAlt& g) {
    for (auto& kv : g.axes)
        if (growth_of(d, kv.first) < kv.second) return "gate.axis|axis=" + kv.first + "|n=" + num(kv.second);
    if (!g.talents.empty()) {
        bool any = false;
        for (auto& t : g.talents) if (contains(d.talents, t)) any = true;
        if (!any) return "gate.talent|talent=" + g.talents.front();
    }
    if (g.phase > 0 && d.phase < g.phase) return "gate.phase|n=" + std::to_string(g.phase);
    return "";
}

bool has_target(const Act& a) { return !a.target.empty() || !a.region.empty() || !a.item.empty() || a.target_kind == "lair"; }

}

const ActRule* act_rule(const Params& p, const std::string& type) {
    auto it = p.act_book.acts.find(type);
    return it == p.act_book.acts.end() ? nullptr : &it->second;
}

std::string act_fields_refusal(const World& w, const Params& p, const Act& a) {
    (void)w;
    const ActRule* r = act_rule(p, a.type);
    if (!r) return "act.unknown|type=" + a.type;
    if (!r->target.empty() && !has_target(a)) return "act.malformed|field=target";
    if (!a.target_kind.empty() && !r->target.empty() && !contains(r->target, a.target_kind)) return "act.bad_value|field=target.kind|value=" + a.target_kind;
    if (r->secondary.size() == 1 && r->secondary.front() == "faction" && a.against.empty()) return "act.malformed|field=secondary";
    if (a.type == "burn" && !a.item.empty()) {
        static const char* const s[] = {"library", "temple", "foundry", "walls", "altar", "granary", "tower"};
        bool ok = false;
        for (const char* k : s) if (a.item == k) ok = true;
        if (!ok) return "act.bad_value|field=secondary.id|value=" + a.item;
    }
    for (auto& kv : r->args) {
        auto it = a.args.find(kv.first);
        if (it == a.args.end() || it->second.empty()) return "act.malformed|field=payload." + kv.first;
        if (!kv.second.empty() && !contains(kv.second, it->second)) return "act.bad_value|field=payload." + kv.first + "|value=" + it->second;
    }
    for (auto& k : r->numbers) if (!a.payload.count(k)) return "act.malformed|field=payload." + k;
    return "";
}

std::string gates_refusal(const World& w, const Params& p, const std::vector<GateAlt>& gates) {
    (void)p;
    if (gates.empty()) return "";
    std::string first;
    for (auto& g : gates) {
        std::string why = alt_refusal(w.dragons[kPlayer], g);
        if (why.empty()) return "";
        if (first.empty()) first = why;
    }
    return first;
}

std::string act_gate_refusal(const World& w, const Params& p, const Act& a) {
    const ActRule* r = act_rule(p, a.type);
    if (!r) return "";
    std::string why = gates_refusal(w, p, r->gates);
    if (!why.empty()) return why;
    if (r->once)
        for (size_t i = w.acts.size(); i > 0; --i) {
            const Act& x = w.acts[i - 1];
            if (x.year < w.year) break;
            if (x.type == a.type) return "gate.once";
        }
    for (auto& kv : r->pool)
        if (pool_of(w.dragons[kPlayer], kv.first) < kv.second) return "gate.pool|pool=" + kv.first + "|n=" + num(kv.second);
    return "";
}

void pay_act(World& w, const Params& p, const Act& a) {
    const ActRule* r = act_rule(p, a.type);
    if (!r) return;
    Dragon& d = w.dragons[kPlayer];
    for (auto& kv : r->pool) {
        double& v = kv.first == "magic" ? d.magic : kv.first == "knowledge" ? d.knowledge : d.faith_pool;
        v = std::max(0.0, v - kv.second);
    }
}

bool act_secret(const Params& p, const Act& a) {
    if (a.type == "corrupt") return !a.item.empty();   // on a person (normalise_act put the person in item)
    const ActRule* r = act_rule(p, a.type);
    return r && r->secret;
}

}

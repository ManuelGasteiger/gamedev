#pragma once
#include <vector>
#include "ages/ages.h"
#include "ages/world_util.h"
#include "ages/rules/rules.h"
#include "check.h"

inline ages::LoadResult load_or_die(uint64_t seed) {
    ages::LoadResult r = ages::load_data(AGES_DATA_DIR, seed);
    CHECK(r.ok, r.error.c_str());
    return r;
}

inline ages::Act make_act(const char* type, const char* target, double m, const char* region = "") {
    ages::Act a;
    a.type = type;
    a.target = target;
    a.magnitude = m;
    a.region = region;
    return a;
}

inline std::vector<ages::Act> sample_log() {
    std::vector<ages::Act> log;
    log.push_back(make_act("burn", "aldmere", 0.4, "east_vale"));
    log.back().witnesses = {"vessary", "dorun", "ilvareth"};
    ages::Act take = make_act("take", "vessary", 0.3);
    take.payload["gold"] = 1.0;
    take.payload["royals"] = 1.0;
    log.push_back(take);
    log.push_back(make_act("show", "dorun", 0.2));
    log.push_back(make_act("give", "ilvareth", 0.2));
    return log;
}

inline ages::Oath make_oath(const char* faction, const char* pred, const char* who = "dragon") {
    ages::Oath o;
    o.id = std::string("oath_") + faction;
    o.parties[0] = "dragon";
    o.parties[1] = faction;
    o.sworn = 0;
    o.expires = 100000;
    ages::Clause cl;
    cl.who = who;
    cl.pred = pred;
    o.clauses.push_back(cl);
    o.breach_by_dragon = "betrayal";
    o.breach_by_them = "none";
    return o;
}

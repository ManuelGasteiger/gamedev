#include <cmath>
#include "fixtures.h"
#include "ages/memory_tiers.h"
#include "ages/moves.h"
#include "ages/person_record.h"

// research-social-sim.md section 6, the corrupted-princess chain, on the fixed start world

using namespace ages;

namespace {

const char* kCourt = "ilvareth";   // elves, so the queen outlives the 60 years and the secret

bool has_mem(const Person& x, const std::string& ev) {
    for (auto* v : {&x.mem, &x.permanent})
        for (auto& m : *v) if (m.ev == ev) return true;
    return false;
}

int fired(const World& w, const std::string& kind, const std::string& object) {
    for (auto& e : w.chronicle) if (e.kind == kind && e.object == object) return e.year;
    return 0;
}

}

int main() {
    LoadResult r = load_or_die(1);
    World& w = r.world;
    const Params& p = r.params;
    for (int i = 0; i < 50 && !find_person_role(w, kCourt, "heir"); ++i) sleep(w, p, 10);
    Faction& court = *find_faction(w, kCourt);
    Person* edric = find_person_role(w, kCourt, "ruler");
    Person* ilsa = find_person_role(w, kCourt, "heir");
    CHECK(edric && ilsa, "the court has a ruler and an heir");
    std::string king = edric->id, heir = ilsa->id;

    edric->traits.list = {"craven"};
    edric->dies = w.year + 50;
    PersonMemory burned;
    burned.ev = "act_burn";
    burned.sev = 3;
    burned.dread = 1;
    edric->mem.push_back(burned);
    ilsa->traits.list = {"curious", "ambitious"};
    ilsa->born = w.year - 6;
    ilsa->dies = w.year + 200;
    court.dread.oral = 0.6;
    court.culture.traits.piety = 1.0;   // a devout court: the purge needs one
    Arrival envoy;
    envoy.id = "envoy_edric";
    envoy.kind = "envoy";
    envoy.court = kCourt;
    envoy.person = king;
    w.arrivals.push_back(envoy);

    MoveCtx ctx;
    ctx.growth = {{"lore", 5}, {"arcana", 3}};
    ctx.demand = "hostage";
    MoveOutcome threat = resolve_move(w, p, king, "threaten", ctx);
    CHECK(threat.refusal.empty(), "Edric is at the lair as an envoy");
    CHECK(threat.accepted && threat.score == 7, "Threaten the craven king: 7 against 1, he complies");
    CHECK(has_mem(*find_person(w, king), "taken_hostage"), "Edric remembers giving up his heir (sev 3)");
    ilsa = find_person(w, heir);
    CHECK(has_tag(ilsa->tags, "hostage") && has_mem(*ilsa, "taken"), "Ilsa is a hostage in the lair and remembers being taken");
    CHECK(court.dread.oral > 0.6, "the court heard of the threat (oral rumour)");

    ctx.subject = "letters";
    MoveOutcome taught = resolve_move(w, p, heir, "teach", ctx);
    CHECK(taught.accepted && taught.score == 5, "Teach the curious heir: 5 against 2");
    ilsa = find_person(w, heir);
    CHECK(has_tag(ilsa->tags, "lettered") && has_mem(*ilsa, "taught"), "Ilsa is lettered and remembers being taught");
    CHECK(axes(p, *ilsa).debt == 1, "teaching left debt 1");

    MoveOutcome corrupted = resolve_move(w, p, heir, "corrupt", ctx);
    CHECK(corrupted.accepted && corrupted.score == 5, "Corrupt after Teach: 5 against 3");
    ilsa = find_person(w, heir);
    CHECK(has_tag(ilsa->tags, "corrupted:dragon"), "Ilsa carries corrupted:dragon");
    CHECK(w.secrets.size() == 1 && w.secrets[0].about == heir && w.secrets[0].holder == kCourt, "a secret about Ilsa is held by her court");

    apply_acts(w, p, {make_act("release", kCourt, 1.0)});
    ilsa = find_person(w, heir);
    CHECK(!has_tag(ilsa->tags, "hostage") && has_mem(*ilsa, "mercy"), "Release reads mercy");
    Axes a = axes(p, *ilsa);
    CHECK(a.debt == 3 && a.awe == 3, "Ilsa leaves with debt 3 and awe 3");

    int start = w.year;
    sleep(w, p, 10);
    double sages = 0;
    for (auto& x : w.people) if (x.faction == kCourt && x.role == "sage") ++sages;
    double rate = 0.05 * (find_faction(w, kCourt)->culture.traits.piety + sages * 0.02);
    CHECK(!w.secrets.empty() && std::fabs(w.secrets[0].discovery_per_decade - rate) < 1e-12, "discovery per decade is 0.05 x (piety + sages x 0.02)");
    bool rules = false;
    for (int y = 10; y < 60 && !rules; y += 10) {
        sleep(w, p, 10);
        ilsa = find_person(w, heir);
        rules = ilsa && ilsa->role == "ruler";
    }
    CHECK(rules, "within 60 years she rules");
    const Faction* hers = find_faction(w, ilsa->faction);
    CHECK(hers && memory_tier(*hers, p.c.mem) == "written", "her court keeps its memory of the dragon in writing");

    int exposed = fired(w, "secret_exposed", heir);
    for (int i = 0; i < 100 && !exposed; ++i) {
        sleep(w, p, 50);
        exposed = fired(w, "secret_exposed", heir);
    }
    CHECK(exposed, "the secret surfaces");
    sleep(w, p, 20);
    int inquisition = fired(w, "inquisition", heir);
    CHECK(inquisition >= exposed, "the inquisition card fires on the exposed secret");
    std::printf("princess_chain ok (secret found %d years after the release, inquisition in %d)\n", exposed - start, inquisition);
    return 0;
}

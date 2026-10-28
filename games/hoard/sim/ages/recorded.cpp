#include "record.h"
#include "unrecorded.h"
#include "arrivals.h"
#include "endings.h"
#include "chron.h"

namespace ages {

std::vector<ReactionLine> apply_acts(World& w, const Params& p, const std::vector<Act>& acts) {
    RecordScope rec(w);
    auto out = apply_acts_unrecorded(w, p, acts);
    rec.done("apply_acts", args_acts(acts));
    return out;
}

SleepResult sleep(World& w, const Params& p, int years) {
    RecordScope rec(w);
    SleepResult r = sleep_unrecorded(w, p, years);
    rec.done("sleep", args_int("years", years));
    return r;
}

bool pick_talent(World& w, const Params& p, const std::string& id, std::string& reason) {
    RecordScope rec(w);
    bool ok = pick_talent_unrecorded(w, p, id, reason);
    if (ok) rec.done("pick_talent", args_of({{"id", id}}));
    return ok;
}

bool accept_offer(World& w, const Params& p, size_t index) {
    RecordScope rec(w);
    bool ok = accept_offer_unrecorded(w, p, index);
    if (ok) rec.done("accept_offer", args_int("index", static_cast<long long>(index)));
    return ok;
}

bool decline_offer(World& w, const Params& p, size_t index) {
    RecordScope rec(w);
    bool ok = decline_offer_unrecorded(w, p, index);
    if (ok) rec.done("decline_offer", args_int("index", static_cast<long long>(index)));
    return ok;
}

DemandResult demand_oath(World& w, const Params& p, const std::string& target, const std::string& pred) {
    RecordScope rec(w);
    DemandResult r = demand_oath_unrecorded(w, p, target, pred);
    rec.done("demand", args_of({{"target", target}, {"pred", pred}}));   // a refused demand still hardens the court
    return r;
}

bool answer_hook(World& w, const Params& p, const std::string& id, const std::string& choice, std::string& reason) {
    RecordScope rec(w);
    bool ok = answer_hook_unrecorded(w, p, id, choice, reason);
    if (ok) rec.done("answer_hook", args_of({{"id", id}, {"choice", choice}}));
    return ok;
}

bool kill_person(World& w, const Params& p, const std::string& id, std::string& reason, std::vector<ReactionLine>* lines) {
    RecordScope rec(w);
    bool ok = kill_person_unrecorded(w, p, id, reason, lines);
    if (ok) rec.done("kill_person", args_of({{"id", id}}));
    return ok;
}

bool receive(World& w, const Params& p, const std::string& id, const std::string& outcome, std::string& reason) {
    RecordScope rec(w);
    bool ok = receive_unrecorded(w, p, id, outcome, reason);
    if (ok) rec.done("receive", args_of({{"id", id}, {"outcome", outcome}}));
    return ok;
}

bool hunt(World& w, const Params& p, const std::string& region, std::string& reason) {
    RecordScope rec(w);
    bool ok = hunt_unrecorded(w, p, region, reason);
    if (ok) rec.done("hunt", args_of({{"region", region}}));
    return ok;
}

bool exit_run(World& w, const Params& p, const std::string& kind, std::string& reason) {
    RecordScope rec(w);
    bool ok = exit_unrecorded(w, p, kind, reason);
    if (ok) rec.done("exit", args_of({{"kind", kind}}));
    return ok;
}

CastResult cast_spell(World& w, const Params& p, const std::string& spell, const std::string& target,
                      const std::string& region, const std::string& against) {
    RecordScope rec(w);
    CastResult r = cast_spell_unrecorded(w, p, spell, target, region, against);
    if (r.cast) rec.done("cast", args_of({{"spell", spell}, {"target", target}, {"region", region}, {"against", against}}));
    return r;
}

TerraformResult terraform(World& w, const Params& p, const std::string& region, const std::string& kind) {
    RecordScope rec(w);
    TerraformResult r = terraform_unrecorded(w, p, region, kind);
    if (r.done) rec.done("terraform", args_of({{"region", region}, {"kind", kind}}));
    return r;
}

bool keep_person(World& w, const Params& p, const std::string& id, const std::string& role, std::string& reason) {
    RecordScope rec(w);
    bool ok = keep_person_unrecorded(w, p, id, role, reason);
    if (ok) rec.done("keep_person", args_of({{"id", id}, {"role", role}}));
    return ok;
}

bool commune(World& w, const Params& p, const std::string& relic, const std::string& answer, std::string& reason) {
    RecordScope rec(w);
    bool ok = commune_unrecorded(w, p, relic, answer, reason);
    if (ok) rec.done("commune", args_of({{"relic", relic}, {"answer", answer}}));
    return ok;
}

}

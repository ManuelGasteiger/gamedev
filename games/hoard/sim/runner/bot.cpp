#include "bot.h"
#include <memory>
#include "ages/endings.h"
#include "ages/arrivals.h"
#include "ages/world_util.h"

using namespace ages;

bool parse_bot(const std::string& spec, Bot& out) {
    size_t colon = spec.find(':');
    out.kind = spec.substr(0, colon);
    out.target = colon == std::string::npos ? "" : spec.substr(colon + 1);
    return out.kind == "amnesiac" || out.kind == "carry" || (out.kind == "oracle" && !out.target.empty());
}

// carry: the enabled ending with the most known atoms that it has not used yet (all used: the most known)
std::string bot_target(const Bot& bot, const Profile& pr, const Params& p) {
    if (bot.kind == "oracle") return bot.target;
    if (bot.kind != "carry") return "";
    std::string best;
    int best_known = 0;
    for (auto& e : p.endings.endings) {
        if (!e.enabled) continue;
        int known = 0;
        bool used = false;
        for (auto& c : e.conds) {
            if (atom_known(pr, e.id, c.id)) known += 1;
            auto it = pr.atoms.find(e.id);
            if (it != pr.atoms.end() && it->second.count(c.id) && it->second.at(c.id).state == "used") used = true;
        }
        int score = used ? 0 : known;
        if (score > best_known) { best_known = score; best = e.id; }
    }
    return best;
}

void attach_bot(Scenario& s, const Params& p, const std::string& target) {
    auto steer = std::make_shared<Policy>();
    bool steering = false;
    if (!target.empty() && target != "myth") {
        std::string err;
        steering = load_policy(s.data_dir, "steer_" + target, *steer, err);
    }
    auto previous = s.at_waking;
    std::string data_dir = s.data_dir;
    s.at_waking = [=, &p](World& w, std::vector<Act>& acts, int index) {
        if (previous) previous(w, acts, index);
        for (auto& e : p.endings.endings) {
            if (!e.enabled || e.exit_kind.empty() || !ending_ready(w, e.id)) continue;
            std::string reason;
            if (exit_run(w, p, e.exit_kind, reason)) { acts.clear(); return; }
        }
        if (!steering) return;
        acts = resolve_policy(w, p, *steer, index, data_dir);
        if (target != "mortality") return;
        std::vector<std::string> waiting;
        for (auto& a : w.arrivals) waiting.push_back(a.id);
        for (auto& id : waiting) { std::string reason; receive(w, p, id, "receive", reason); }
    };
}

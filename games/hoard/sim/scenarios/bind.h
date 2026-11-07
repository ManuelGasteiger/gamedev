#pragma once
#include <functional>
#include <string>
#include "fields.h"

namespace scn {

// spec section 3: binds every role of x.d into x.bind, dependencies first, backtracking over each role's candidates
// (at most candidates_per_role, in `choose` order) until require and forbid hold; optional roles may stay unbound.
// cite_ok says whether an act may still be cited (the citation budget). false when no binding exists.
bool bind_roles(Ctx& x, const std::function<bool(int)>& cite_ok, int waking);

// an act pattern (role binder or forbid) against one act; roles it names are read from x.bind
bool act_matches(const Ctx& x, const nlohmann::json& pat, int act);

// why bind_roles failed: the first role (greedy, in binding order) with no candidate, else "require or forbid"
std::string bind_why(Ctx& x);

bool remembers(const ages::World& w, const ages::Params& p, const ages::Faction& f, int act);

}

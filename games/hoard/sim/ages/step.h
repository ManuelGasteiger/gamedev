#pragma once
#include "data.h"
#include "state.h"

namespace ages {

// one step of dt years (17.4): dt == step_years is the full decade step; below a decade the slow rules are skipped
void step(World& w, const Params& p, int dt);
inline void decade_step(World& w, const Params& p) { step(w, p, p.c.step_years); }
double digest_hoard(World& w, const Params& p, double years);
double hunger_step(World& w, const Params& p, double nourishment);
void feed(World& w, const Params& p, const Act& a, double m);
double hunger_need(const World& w, const Constants& c);

}

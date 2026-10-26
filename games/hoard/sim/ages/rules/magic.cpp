#include "world_rules.h"
#include "../world_util.h"

// world.magic: from `start` it rises toward the cap while the world carries the tag age_of_magic and ebbs otherwise;
// the magic cards (necromancers, colleges, wild surges, the king who stopped dying) read it
namespace ages {

void rule_magic(World& w, const Params& p) {
    const MagicDef& m = p.genesis.magic;
    double k = dts(w, p.c);
    if (has_tag(w.tags, "age_of_magic")) w.magic = std::min(m.cap, std::max(w.magic, m.start) + m.rise * k);
    else w.magic = std::max(0.0, w.magic - m.ebb * k);
}

}

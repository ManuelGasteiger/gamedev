#include "rules.h"
#include "../chronicle.h"
#include "../importance.h"

namespace ages {

void rule_chronicle(World& w, const Params& p) {
    compact_chronicle(w.chronicle, w.year);
    score_events(w, p);
}

}

#include "rules.h"
#include "../chronicle.h"
#include "../importance.h"
#include "../../sift/sift.h"

namespace ages {

void rule_chronicle(World& w, const Params& p) {
    compact_chronicle(w.chronicle, w.year);
    score_events(w, p);
    sift_events(w, p);   // item 14
}

}

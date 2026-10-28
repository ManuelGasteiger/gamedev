#include "fixtures.h"
#include "ages/endings.h"

using namespace ages;

namespace {

// met[i] is the condition at year 10 * i
bool ready_after(const std::vector<bool>& met) {
    EndingCond c;
    c.hold_years = 150;
    c.lapse_years = 30;
    CondHold h;
    int year = 0;
    for (bool m : met) { advance_hold(h, m, year, c); year += 10; }
    return cond_held(h, year - 10, c);
}

std::vector<bool> spans(std::initializer_list<std::pair<bool, int>> parts) {
    std::vector<bool> out;
    for (auto& s : parts) for (int y = 0; y < s.second; y += 10) out.push_back(s.first);
    return out;
}

}

int main() {
    // held 140 years (15 ticks from year 0), lapsed 40, held 20: the lapse reset the timer
    CHECK(!ready_after(spans({{true, 150}, {false, 40}, {true, 30}})), "140 held, 40 lapsed, 20 held must not be ready");
    // held 150 years with a lapse of 20 inside: ready
    CHECK(ready_after(spans({{true, 90}, {false, 20}, {true, 50}})), "150 held with a 20-year lapse must be ready");
    std::printf("ending_continuity ok\n");
    return 0;
}

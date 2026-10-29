#pragma once
#include <string>
#include <vector>
#include "ages/data.h"

// item 14: the sifter. The chronicle is the chronicler, the pattern bank the sifter, the briefing and the court
// sources the narrativizers (research-story-sifting.md section 1).

namespace ages {

using ThreadLine = SiftLine;

// one court's telling of one thread (research section 6); event ids
struct Source {
    int match = 0;
    std::string court, medium;   // medium: oral | written | monumental
    int start = 0;
    std::vector<int> omit, passive, printed;
    std::string blame, stated_cause, archetype;
    int hedge = 0;
};

void sift_events(World& w, const Params& p);              // acceptors, at the end of every step
void sift_wake(World& w, const Params& p, int since_year);  // picks a waking's lines and records them as shown

// the lines for a sleep since since_year, picked without touching the world; budget_override replaces L(y)
std::vector<ThreadLine> briefing_lines(const World& w, const Params& p, int since_year, int budget_override = -1);
int line_budget(int years);   // L(y) = min(9, 3 + floor(2 log10 y))

// Labov's chain: from the event back through its causes, stopping after the first unremarkable one
std::vector<int> because_chain(const World& w, const Params& p, int event, int hops);

Source make_source(const World& w, const Params& p, int match, const std::string& court);
Source make_event_source(const World& w, const Params& p, int event, const std::string& court);
bool sources_conflict(const World& w, const Source& a, const Source& b);

double thread_score(const World& w, const Params& p, const SiftThread& t);

}

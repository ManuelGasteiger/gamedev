#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "state.h"

namespace ages {

void compact_chronicle(std::vector<Event>& events, int year);
std::string canonical_event(const Event& e);
uint64_t fnv1a(const std::string& s, uint64_t h = 1469598103934665603ull);
uint64_t hash_events(const std::vector<Event>& events);
std::string events_to_json(const std::vector<Event>& events);

}

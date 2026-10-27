#pragma once
#include "ages/data.h"
#include "ages/json.h"

namespace ages {

nlohmann::json event_json(const Event& e);    // serialise.cpp
Event event_from(const nlohmann::json& j);

nlohmann::json sift_state_json(const SiftState& s);
void sift_state_from_json(const nlohmann::json& j, SiftState& s);
nlohmann::json sift_view_json(const World& w, const Params& p);   // open threads and the ones that ended this waking
nlohmann::json sift_lines_json(const std::vector<SiftLine>& lines);

}

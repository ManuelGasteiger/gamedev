#pragma once
#include "ages/json.h"
#include "ages/state.h"

// item 08: the slayer roster of a finished run for stats.json: the orders with their grandmasters and the counts the acceptance reads
namespace ages {

nlohmann::json roster_json(const World& w);

}

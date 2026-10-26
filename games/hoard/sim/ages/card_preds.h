#pragma once
#include <string>
#include <vector>
#include "cards.h"
#include "json.h"

// v0.4 hooks (S3): the card trigger language parsed from a JSON array outside a card (data/hooks.json `when`)

namespace ages {

bool card_preds_from_json(const nlohmann::json& arr, std::vector<CardPred>& out, std::string& err);   // card_data.cpp

}

#pragma once
#include <map>
#include <string>
#include <vector>
#include "../cards.h"
#include "../data.h"
#include "../state.h"

namespace ages {

// what a card is evaluated against: named variables ($self, $other, $region, $owner, $person, $arg, $new, ...) bound by
// the trigger, the events that satisfied it (the card event's cause refs) and, once fired, the card event itself
struct CardCtx {
    const CardDef* card = nullptr;
    std::map<std::string, std::string> vars;
    std::vector<int> refs;
    int event = 0;
    const std::map<std::string, int>* last_act = nullptr;   // faction -> year of the newest dragon act on it
    int any_act = 0;                                        // year of the newest dragon act anywhere
};

std::string var_of(const CardCtx& ctx, const std::string& name);   // "other", "$other", or the literals world | dragon | lair
std::string subst(const CardCtx& ctx, const std::string& s);       // "grudge:$other" -> "grudge:aldmere"
void add_refs(CardCtx& ctx, const std::vector<int>& refs);

// tags of any entity id: a faction, a region, a person, "world" or "dragon"
Tags* tags_of(World& w, const std::string& id);
const Tags* tags_of(const World& w, const std::string& id);
const TimedTag* timed_tag(const World& w, const std::string& target, const std::string& tag);
int newest_event(const World& w, const std::string& kind, const std::string& subject);   // "act_*" matches every act

// card_match.cpp: true when every condition holds; binds variables and appends refs to ctx
bool card_holds(const World& w, const Params& p, const std::vector<CardPred>& preds, CardCtx& ctx);

// v0.4 hooks (S3): a numeric field of any entity, as the trigger language reads it ("wealth" of a court, "bold" of a person)
bool card_num_field(const World& w, const Params& p, const CardCtx& ctx, const std::string& id, const std::string& key, double& v);

// card_effects.cpp
void apply_card_effect(World& w, const Params& p, const CardEffect& e, CardCtx& ctx);
void write_tag(World& w, const std::string& target, const std::string& tag, int decades, bool inherit, int event);
void remove_tags(World& w, const std::string& target, const std::string& pattern);   // "grudge:*" removes the family

// card_world.cpp (v0.4 world, S2): arrive, spawn_undead, terrain, undying; false for any other effect
bool world_effect(World& w, const Params& p, const CardEffect& e, CardCtx& ctx);
// cards.cpp (v0.4 world, S2): the storyteller fires a card without its chance roll, on one self where its trigger holds
bool fire_card_now(World& w, const Params& p, const CardDef& card);

}

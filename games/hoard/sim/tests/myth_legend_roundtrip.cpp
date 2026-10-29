#include <filesystem>
#include "fixtures.h"
#include "ages/genesis.h"
#include "ages/memory_tiers.h"
#include "ages/profile.h"

using namespace ages;

int main() {
    LoadResult r = load_or_die(42);
    World& w = r.world;
    const Params& p = r.params;
    CHECK(w.factions.size() >= 2, "fixed start has courts");
    w.year = 900;
    for (int i = 0; i < 2; ++i) {   // two temples, so two courts keep the dragon in stone
        Faction& f = w.factions[static_cast<size_t>(i)];
        Monument m;
        m.region = f.regions.front();
        m.bias = "dragon_temple";
        m.built = 300;
        m.awe = 0.6;
        f.monuments.push_back(m);
        refresh_monumental(f);
    }
    for (int i = 0; i < 3; ++i) {   // three deeds kept in writing
        Act a = make_act("give", w.factions[0].id.c_str(), 0.5);
        a.year = 100 * (i + 1);
        a.witnesses = {w.factions[0].id, w.factions[1].id};
        w.acts.push_back(a);
        MemoryEntry e;
        e.year = a.year;
        e.type = "give";
        e.axis = "awe";
        e.tier = "written";
        e.act = static_cast<int>(w.acts.size()) - 1;
        w.factions[0].memory.push_back(e);
    }
    w.tags.insert("dragon_slain");
    emit(w, "dragon_slain", w.factions[1].id, "", 1, {}, because("slaying"));

    Profile pr;
    note_death(pr, w, p);
    CHECK(pr.legends.size() == 1, "a qualifying death writes one legend");
    CHECK(pr.legends[0].reign == 900 && pr.legends[0].killer == w.factions[1].id, "the legend names its reign and killer");

    std::string dir = (std::filesystem::temp_directory_path() / "hoard_myth_legend_roundtrip").string();
    std::filesystem::remove_all(dir);
    std::string err;
    CHECK(save_profile(dir, pr, err), err.c_str());
    CHECK(std::filesystem::exists(dir + "/legends.json"), "legends.json written");
    Profile back;
    CHECK(load_profile(dir, back, err), err.c_str());
    CHECK(back.legends.size() == 1, "legends.json reads back");

    LoadResult fresh = load_or_die(7);
    Params q = fresh.params;
    q.endings.legends.chance = 1.0;   // the test weaves the legend for certain
    CHECK(make_genesis(fresh.world, q, back, err), err.c_str());
    int tags = 0;
    for (auto& t : fresh.world.tags) if (t.rfind("legend:", 0) == 0) tags += 1;
    CHECK(tags == 1, "genesis adds exactly one legend tag");
    bool line = false;
    for (auto& e : fresh.world.chronicle) if (e.tags.count("src:legend") && e.tags.count("prehistory")) line = true;
    CHECK(line, "the pre-history tells of the legend");
    std::filesystem::remove_all(dir);
    std::printf("myth_legend_roundtrip ok\n");
    return 0;
}

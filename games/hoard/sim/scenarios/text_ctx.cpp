#include "text_ctx.h"
#include "ages/names.h"
#include "ages/world_util.h"

namespace scn {

using namespace ages;

text::Ctx text_ctx(const Ctx& x, const std::string& lang) {
    text::Ctx c;
    for (auto& b : x.bind) {
        const Role* r = x.d.role(b.first);
        if (!r) continue;
        text::Entity e;
        e.kind = r->kind;
        if (r->kind == "person") {
            const Person* p = find_person(x.w, b.second);
            e.name = p ? p->name : display_name(x.w, b.second);
            e.noun = r->noun;
            e.sex = p && !p->sex.empty() ? p->sex : "m";
        } else if (r->kind == "act") {
            e.fields["place"] = field(x, b.first, "place").str;
            e.fields["target_name"] = field(x, b.first, "target_name").str;
            e.fields["verb"] = field(x, b.first, "verb").str;
            std::string size = field(x, b.first, "size").str;
            if (!size.empty() && x.p.sift.texts) e.fields["size"] = text::render_key(*x.p.sift.texts, lang, "scn.size." + size, text::Ctx());
            e.numbers["years_ago"] = static_cast<long long>(field(x, b.first, "years_ago").num);
            e.numbers["wakings_ago"] = static_cast<long long>(field(x, b.first, "wakings_ago").num);
        } else if (r->kind == "item") {
            e.name = field(x, b.first, "name").str;
            e.gender = "n";
        } else e.name = display_name(x.w, b.second);
        c.roles[b.first] = e;
    }
    return c;
}

}

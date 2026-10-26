#include "reason.h"
#include "numbers.h"
#include "render.h"

namespace text {

std::string reason_text(const Bundles& all, const std::string& lang, const std::string& encoded) {
    const Bundle* b = all.get(lang);
    if (!b) b = all.get("en");
    if (!b) return encoded;
    Entity r;
    r.kind = "reason";
    std::string code = encoded.substr(0, encoded.find('|'));
    for (size_t at = encoded.find('|'); at != std::string::npos;) {
        size_t next = encoded.find('|', at + 1);
        std::string kv = encoded.substr(at + 1, next == std::string::npos ? std::string::npos : next - at - 1);
        size_t eq = kv.find('=');
        if (eq != std::string::npos) {
            std::string k = kv.substr(0, eq), v = kv.substr(eq + 1), shown;
            if (!v.empty() && format_decimal(v, part(*b, "locale"), shown)) v = shown;
            else if (const std::string* word = find_string(*b, "word." + v)) v = *word;
            r.fields[k] = v;
        }
        at = next;
    }
    Ctx c;
    c.roles["r"] = r;
    return render_key(all, b->lang, "reason." + code, c);
}

}

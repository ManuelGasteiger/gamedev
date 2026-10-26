#include "chronicle.h"
#include <algorithm>
#include <cstdio>
#include "json.h"

namespace ages {

void compact_chronicle(std::vector<Event>& events, int year) {
    size_t start = events.size();
    while (start > 0 && events[start - 1].year == year) --start;
    std::vector<Event> out(events.begin(), events.begin() + static_cast<std::ptrdiff_t>(start));
    for (size_t i = start; i < events.size(); ++i) {
        const Event& e = events[i];
        bool merged = false;
        for (size_t j = start; j < out.size(); ++j) {
            Event& o = out[j];
            if (o.kind == e.kind && o.subject == e.subject && o.object == e.object && o.tags == e.tags) {
                o.count += 1;
                o.magnitude += e.magnitude;
                for (int r : e.cause.refs) if (std::find(o.cause.refs.begin(), o.cause.refs.end(), r) == o.cause.refs.end()) o.cause.refs.push_back(r);
                std::sort(o.cause.refs.begin(), o.cause.refs.end());
                merged = true;
                break;
            }
        }
        if (!merged) out.push_back(e);
    }
    events.swap(out);
}

// ids and cause refs are part of the hash; the cause's text key and params are not, so phrasing can change freely
std::string canonical_event(const Event& e) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%d|%d|%.6f|%d|", e.id, e.year, e.magnitude, e.count);
    std::string s = buf;
    s += e.kind + "|" + e.subject + "|" + e.object + "|";
    for (auto& t : e.tags) { s += t; s += ','; }
    s += '|';
    for (int r : e.cause.refs) { s += std::to_string(r); s += ','; }
    return s;
}

uint64_t fnv1a(const std::string& s, uint64_t h) {
    for (unsigned char ch : s) {
        h ^= ch;
        h *= 1099511628211ull;
    }
    return h;
}

uint64_t hash_events(const std::vector<Event>& events) {
    uint64_t h = 1469598103934665603ull;
    for (auto& e : events) h = fnv1a(canonical_event(e) + "\n", h);
    return h;
}

// the text golden: one event per line, year kind subject object magnitude cause, readable in a diff
std::string chronicle_text(const std::vector<Event>& events) {
    std::string out;
    char mag[32];
    for (auto& e : events) {
        std::snprintf(mag, sizeof mag, "%.6g", e.magnitude);
        out += std::to_string(e.year) + " " + e.kind + " " + (e.subject.empty() ? "-" : e.subject) + " " +
               (e.object.empty() ? "-" : e.object) + " " + mag + " " + (e.cause.text_key.empty() ? "-" : e.cause.text_key);
        if (e.count > 1) out += " x" + std::to_string(e.count);
        out += "\n";
    }
    return out;
}

std::string events_to_json(const std::vector<Event>& events) {
    nlohmann::json arr = nlohmann::json::array();
    for (auto& e : events) {
        nlohmann::json j;
        j["id"] = e.id;
        j["year"] = e.year;
        j["kind"] = e.kind;
        j["subject"] = e.subject;
        j["object"] = e.object;
        j["magnitude"] = e.magnitude;
        j["text_key"] = e.text_key;
        j["tags"] = e.tags;
        j["count"] = e.count;
        j["cause"] = {{"refs", e.cause.refs}, {"text_key", e.cause.text_key}, {"params", e.cause.params}};
        j["thread"] = e.thread;
        j["importance"] = e.importance;
        arr.push_back(j);
    }
    return arr.dump(1);
}

}

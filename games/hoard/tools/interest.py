#!/usr/bin/env python3
"""Interest metrics for the ages sim (doc section 17.6).

  interest.py --bin build/hoard-sim --data data --seeds 1-1000 --years 3000 [--policies idle,raze_all,...] [--out out/interest]
  interest.py --chronicle 42 [--policy played]          # narrated chronicle for blind judging
  interest.py --read-set 3                              # with a run: write top/bottom narrated chronicles to out/interest/read/
  interest.py --genesis ...                             # generated worlds (data/genesis.json): adds the world table below

Per seed and policy: outcome tuple, reversals per 1000 years, causal depth (share of events whose cause chain reaches
>= 3 sleeps back),
pacing (CV of events per century), persistence (long-memory events), dragon agency (Jaccard distance to idle).
Per policy: outcome entropy, means, flatness flags, the five most and least interesting seeds.
v0.4 world table (every run): start variety (templates, terrain deals, the most common template's share), the share of
the most common first three events, the rate of each irreversible event and of undead, quiet stretches between major
events after the hatching, and the storyteller's wakings (majors per 10, breathers, the longest run without a major,
the share whose briefing leads with an event of at least importance_min, sleeps of 50+ years with a major).
"""
import argparse
import json
import math
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from concurrent.futures import ProcessPoolExecutor, as_completed

DEFAULT_POLICIES = ["idle", "raze_all", "protect_one", "alternate", "played"]
WEIGHTS = {"reversals_per_1000y": 1.0, "pacing_cv": 0.5, "persistence": 0.5, "agency": 0.5, "rarity": 0.5, "causal_depth": 1.0}
META_KINDS = {"sleep_begin", "wake"}
ENTITY_SKIP = {"", "dragon", "world"}
LONG_MEMORY_YEARS = 500
VASSAL_REVERSAL_YEARS = 100
BANK_IDS = []   # item 14: the pattern ids of the data dir, set in main


# ---------------------------------------------------------------- running

def parse_range(text):
    if "-" in text:
        a, b = text.split("-", 1)
        return range(int(a), int(b) + 1)
    return range(1, int(text) + 1)


def run_binary(binary, data, seed, years, policy, sleep, out, narrate=False, genesis=False):
    cmd = [binary, "--data", data, "--seed", str(seed), "--years", str(years), "--sleep", str(sleep), "--out", out]
    if policy != "idle":
        cmd += ["--policy", policy]
    if narrate:
        cmd += ["--narrate"]
    if genesis:
        cmd += ["--genesis"]
    os.makedirs(out, exist_ok=True)
    cmd += ["--wakings", os.path.join(out, "wakings.jsonl")]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"seed {seed} policy {policy}: {res.stderr.strip()}")
    with open(os.path.join(out, "chronicle.json")) as f:
        chron = json.load(f)
    with open(os.path.join(out, "world_final.json")) as f:
        world = json.load(f)
    with open(os.path.join(out, "stats.json")) as f:
        stats = json.load(f)
    with open(os.path.join(out, "wakings.jsonl")) as f:
        stats["wakings_log"] = [json.loads(line) for line in f if line.strip()]
    if narrate:   # the sim's own narrator (data/text/en/narrator.json), cause clauses included, one sentence per chronicle event
        with open(os.path.join(out, "narrated.json")) as f:
            for e, text in zip(chron, json.load(f)):
                e["text"] = text
    if os.path.exists(os.path.join(out, "legends.json")):
        with open(os.path.join(out, "legends.json")) as f:
            stats["legends"] = json.load(f).get("events", [])
    return chron, world, stats


def simulate(binary, data, seed, years, policy, sleep, narrate=False, genesis=False):
    tmp = tempfile.mkdtemp(prefix="interest_")
    try:
        return run_binary(binary, data, seed, years, policy, sleep, os.path.join(tmp, "run"), narrate, genesis)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


# ---------------------------------------------------------------- metrics

def region_set(world):
    return {r["id"] for r in world["regions"]}


def faction_races(world):
    races = {f["id"]: f["race"] for f in world["factions"]}
    for g in world.get("graveyard", []):
        races.setdefault(g["id"], g["race"])
    return races


def outcome(world):
    factions = world["factions"]
    by_race = Counter()
    for f in factions:
        by_race[f["race"]] += len(f["regions"])
    largest = max(by_race.items(), key=lambda kv: (kv[1], kv[0]))[0] if by_race and max(by_race.values()) > 0 else "none"
    realm = {}
    for f in factions:
        realm[f["id"]] = len(f["regions"])
    for f in factions:
        for t in f["tags"]:
            if t.startswith("vassal_of:") and t[10:] in realm:
                realm[t[10:]] += len(f["regions"])
    hegemon = "none"
    for f in factions:
        if "hegemon" in f["tags"]:
            hegemon = f["race"]
    leader_era = -1
    if factions:
        lead = max(factions, key=lambda f: (realm[f["id"]], f["wealth"], f["id"]))
        leader_era = lead["era"]
    return {"largest_race": largest, "hegemon_race": hegemon, "leader_era": leader_era, "living_factions": len(factions),
            "dragon_slain": "dragon_slain" in world.get("tags", []), "final_year": world["year"]}


def reversals(events, years, races=None):
    """hegemon fell, vassal freed (incl. revolt) when the vassalage lasted >= 100 years or crossed races, a returned
    faction took a region back, a coalition won."""
    n = Counter()
    returned = set()
    coalitions = {}
    races = races or {}
    sworn = {}
    for e in events:
        k = e["kind"]
        if k in ("hegemony_fell", "hegemony_ended"):
            n["hegemon_fell"] += e["count"]
        elif k == "vassal_sworn":
            sworn[e["subject"]] = (e["year"], e["object"])
        elif k in ("vassal_freed", "vassal_revolt"):
            year, lord = sworn.get(e["subject"], (None, e["object"]))
            long_enough = year is not None and e["year"] - year >= VASSAL_REVERSAL_YEARS
            cross_race = races.get(e["subject"]) is not None and races.get(lord) is not None and races[e["subject"]] != races[lord]
            if long_enough or cross_race:
                n["vassal_freed"] += e["count"]
        elif k == "race_returned":
            returned.add(e["subject"])
            n["returned_took_region"] += e["count"]
        elif k == "region_taken" and e["subject"] in returned:
            n["returned_took_region"] += e["count"]
        elif k == "coalition_formed":
            coalitions[(e["subject"], e["object"])] = {"start": e["year"], "hegemon": e["object"], "end": None}
        elif k == "coalition_ended":
            key = (e["subject"], e["object"])
            if key in coalitions and coalitions[key]["end"] is None:
                coalitions[key]["end"] = e["year"]
    for key, c in coalitions.items():
        end = c["end"] if c["end"] is not None else years
        h = c["hegemon"]
        won = False
        for e in events:
            if e["year"] < c["start"] or e["year"] > end + 10:
                continue
            if e["kind"] == "region_taken" and h in e["tags"]:
                won = True
            elif e["kind"] == "vassal_sworn" and e["subject"] == h:
                won = True
            elif e["kind"] in ("hegemony_fell", "hegemony_ended") and e["subject"] == h:
                won = True
            elif e["kind"] == "collapse" and e["subject"] == h and e["year"] <= end:
                won = True
        if won:
            n["coalition_won"] += 1
    total = sum(n.values())
    return total, dict(n)


def causal_depth(events):
    """share of events whose cause chain reaches >= 3 sleeps back; None when no event carries `cause`."""
    if not any("cause" in e for e in events):
        return None
    by_id = {}
    for i, e in enumerate(events):
        by_id[i] = e
        if "id" in e:
            by_id[e["id"]] = e
    sleeps = sorted(e["year"] for e in events if e["kind"] == "sleep_begin")

    def refs(e):
        c = e.get("cause") or {}
        r = c.get("refs", c if isinstance(c, list) else [])
        return [by_id[x] for x in r if x in by_id]

    def oldest(e, seen):
        best = e["year"]
        for p in refs(e):
            key = id(p)
            if key in seen:
                continue
            seen.add(key)
            best = min(best, oldest(p, seen))
        return best

    deep = 0
    for e in events:
        if e["kind"] in META_KINDS:
            continue
        if "cause" not in e:
            continue
        o = oldest(e, {id(e)})
        crossed = sum(1 for s in sleeps if o < s <= e["year"])
        if crossed >= 3:
            deep += 1
    n = sum(1 for e in events if e["kind"] not in META_KINDS)
    return deep / n if n else 0.0


def pacing(events, years):
    """coefficient of variation of world events per century (sleep/wake and the dragon's own acts excluded)."""
    bins = max(1, (years + 99) // 100)
    counts = [0] * bins
    for e in events:
        if e["kind"] in META_KINDS or e["kind"].startswith("act_"):
            continue
        b = min(bins - 1, e["year"] // 100)
        counts[b] += e["count"]
    mean = sum(counts) / bins
    if mean == 0:
        return 0.0
    sd = statistics.pstdev(counts)
    return sd / mean


def persistence(events, races):
    """share of world events that read something >= 500 years old: a subject/object faction first seen >= 500 y earlier,
    a race returning >= 500 y after its remnant formed, a wasteland healing >= 500 y after forming, a vassal freed
    >= 500 y after swearing, or slayers founded >= 500 y after the dragon first burned that faction."""
    first_seen = {}
    remnant_year = {}
    wasteland_year = {}
    sworn_year = {}
    first_burn = {}
    long_mem = 0
    total = 0
    for e in events:
        k = e["kind"]
        y = e["year"]
        if k in META_KINDS:
            continue
        total += 1
        old = False
        for ent in (e["subject"], e["object"]):
            if ent in ENTITY_SKIP or ent not in races:
                continue
            if ent in first_seen and y - first_seen[ent] >= LONG_MEMORY_YEARS:
                old = True
            first_seen.setdefault(ent, y)
        if k == "remnant_formed":
            remnant_year.setdefault(e["subject"], y)
        elif k == "race_returned":
            race = e["tags"][0] if e["tags"] else ""
            if race in remnant_year and y - remnant_year[race] >= LONG_MEMORY_YEARS:
                old = True
        elif k == "wasteland_formed":
            wasteland_year[e["subject"]] = y
        elif k == "wasteland_healed":
            if e["subject"] in wasteland_year and y - wasteland_year[e["subject"]] >= LONG_MEMORY_YEARS:
                old = True
        elif k == "vassal_sworn":
            sworn_year[e["subject"]] = y
        elif k in ("vassal_freed", "vassal_revolt"):
            if e["subject"] in sworn_year and y - sworn_year[e["subject"]] >= LONG_MEMORY_YEARS:
                old = True
        elif k == "act_burn":
            first_burn.setdefault(e["subject"], y)
        elif k in ("slayers_founded", "slay_attempt_failed", "dragon_slain"):
            if e["subject"] in first_burn and y - first_burn[e["subject"]] >= LONG_MEMORY_YEARS:
                old = True
        if old:
            long_mem += 1
    return long_mem / total if total else 0.0


def signature(events):
    return {(e["year"] // 100, e["kind"], e["subject"]) for e in events
            if e["kind"] not in META_KINDS and not e["kind"].startswith("act_")}


def agency(idle_events, events):
    a, b = signature(idle_events), signature(events)
    union = a | b
    if not union:
        return 0.0
    return 1.0 - len(a & b) / len(union)


def first_three(events):
    setup = events[0]["year"] if events else 0   # the founding instant (world_began, court_founded, vassal_sworn) is the same in every world
    out = []
    for e in events:
        if e["kind"] in META_KINDS or e["kind"].startswith("act_") or e["year"] == setup:
            continue
        out.append((e["kind"], e["subject"]))
        if len(out) == 3:
            break
    return tuple(out)


def is_dragon_act(e):
    return e["kind"].startswith("act_") or e["kind"] == "hook_resolved"


def reaches_dragon(by_id, refs, hops=3):
    frontier, seen = list(refs), set()
    for _ in range(hops):
        nxt = []
        for r in frontier:
            if r in seen or r not in by_id:
                continue
            seen.add(r)
            if is_dragon_act(by_id[r]):
                return True
            nxt += by_id[r]["cause"]["refs"]
        frontier = nxt
    return False


def agency_chain(events, wakings_log, hatched):
    """(a) hooks whose refs reach a dragon act, (b) events after the hatching whose cause chain does, (c) wakings whose
    headline (the most important event since the last waking) does; all within 3 hops."""
    by_id = {e["id"]: e for e in events}
    hooks = [h for w in wakings_log for h in w.get("hooks", [])]
    hook_dragon = sum(1 for h in hooks if reaches_dragon(by_id, h["refs"]))
    played = [e for e in events if e["year"] >= hatched and e["kind"] not in META_KINDS and not is_dragon_act(e)]
    chained = sum(1 for e in played if reaches_dragon(by_id, e["cause"]["refs"]))
    heads, best = [], None
    for e in events:
        if e["year"] < hatched:
            continue
        if e["kind"] == "wake":
            if best:
                heads.append(best)
            best = None
        elif e["kind"] not in META_KINDS and not is_dragon_act(e) and (best is None or e["importance"] >= best["importance"]):
            best = e
    return {"hooks": len(hooks), "hooks_with_refs": sum(1 for h in hooks if h["refs"]), "hooks_dragon": hook_dragon,
            "events": len(played), "events_dragon": chained,
            "headlines": len(heads), "headlines_dragon": sum(1 for e in heads if reaches_dragon(by_id, e["cause"]["refs"]))}


def seed_metrics(events, world, stats, idle_events, numbers=None):
    years = world["year"]
    races = faction_races(world)
    rev_total, rev_parts = reversals(events, years, races)
    per_1000 = rev_total / (years / 1000.0) if years else 0.0
    out = {
        "outcome": outcome(world),
        "reversals": rev_parts,
        "reversals_per_1000y": per_1000,
        "causal_depth": causal_depth(events),
        "pacing_cv": pacing(events, years),
        "persistence": persistence(events, races),
        "agency": agency(idle_events, events) if idle_events is not None else 0.0,
        "events": sum(e["count"] for e in events if e["kind"] not in META_KINDS),
        "first_three": list(first_three(events)),
        "alive": stats.get("alive", {}),
        "years": years,
        "death": stats.get("dragon_death"),   # v0.4 (S1): {cause: wounds | slaying, year, court, warned} or None
        "hooks": stats.get("hooks"),          # v0.4 hooks (S3): wakings, with_hook, offered, answered, by_kind, fights, reward_growth
        "growth": (stats.get("dragon_growth") or {}).get("points"),
        "arrivals": arrivals_count(stats.get("wakings_log", [])),
        "agency_chain": agency_chain(events, stats.get("wakings_log", []), (world.get("genesis") or {}).get("hatched_year", 0)),
        "sift": sift_seed(stats.get("wakings_log", []), world),   # item 14
    }
    if numbers:
        out["world"] = world_metrics(events, world, numbers)
    return out


def worker(args):
    binary, data, seed, years, sleep, policies = args[:6]
    genesis = args[6] if len(args) > 6 else False
    numbers = world_numbers(data)
    results = {}
    idle_events = None
    order = ["idle"] + [p for p in policies if p != "idle"]
    for policy in order:
        events, world, stats = simulate(binary, data, seed, years, policy, sleep, genesis=genesis)
        if policy == "idle":
            idle_events = events
        if policy in policies:
            results[policy] = seed_metrics(events, world, stats, idle_events if policy != "idle" else None, numbers)
    return seed, results


# ---------------------------------------------------------------- v0.4 world (S2)

UNDEAD_KINDS = {"necromancer_rose", "dead_rose", "undead_raised"}
REIGN_ENDS = {"undying_fell", "collapse", "faction_destroyed", "vassal_absorbed"}
IRREVERSIBLE = {"extinct": "race_extinct", "arrived": "people_arrived", "cataclysm": "cataclysm", "magic": "age_of_magic"}


def world_numbers(data):
    """(major importance, briefing importance_min, long-empire years) from the data dir, defaults when absent"""
    major, imp_min, empire = 0.5, 0.25, 1000
    try:
        with open(os.path.join(data, "genesis.json")) as f:
            g = json.load(f)
        major = g.get("storyteller", {}).get("major_importance", major)
        empire = g.get("undying", {}).get("empire_years", empire)
    except OSError:
        pass
    try:
        with open(os.path.join(data, "constants.json")) as f:
            imp_min = json.load(f).get("briefing", {}).get("importance_min", imp_min)
    except OSError:
        pass
    return major, imp_min, empire


def wakings(events):
    """(start year, end year, events) per sleep, from the sleep_begin/wake pairs"""
    out, cur = [], None
    for e in events:
        if e["kind"] == "sleep_begin":
            cur = (e["year"], [])
        elif e["kind"] == "wake" and cur is not None:
            out.append((cur[0], e["year"], cur[1]))
            cur = None
        elif cur is not None:
            cur[1].append(e)
    return out


def longest_empire(events, final_year):
    """the longest undying reign: from undying_rose to its fall, the realm's end, or the end of the run"""
    rose, best = {}, 0
    for e in events:
        if e["kind"] == "undying_rose":
            rose.setdefault(e["subject"], e["year"])
        elif e["kind"] in REIGN_ENDS and e["subject"] in rose:
            best = max(best, e["year"] - rose.pop(e["subject"]))
    for y in rose.values():
        best = max(best, final_year - y)
    return best


def world_metrics(events, world, numbers):
    major, imp_min, empire = numbers
    g = world.get("genesis", {})
    hatch = g.get("hatched_year", 0)
    kinds = Counter(e["kind"] for e in events)
    imp = lambda e: e.get("importance", 0) or 0
    years = sorted({e["year"] for e in events if imp(e) >= major and e["year"] >= hatch and e["kind"] not in META_KINDS})
    marks = [hatch] + years + [world["year"]]
    sleeps = wakings(events)
    majors = [sum(1 for e in ev if imp(e) >= major) for _, _, ev in sleeps]
    run = longest = 0
    for m in majors:
        run = 0 if m else run + 1
        longest = max(longest, run)
    lead = [any(imp(e) >= imp_min and e["year"] >= end - max(10, 0.2 * (end - start)) for e in ev) for start, end, ev in sleeps]
    long_sleeps = [m for (start, end, _), m in zip(sleeps, majors) if end - start >= 50]
    out = {k: kinds[v] > 0 for k, v in IRREVERSIBLE.items()}
    out.update({
        "template": g.get("template", ""),
        "deal": "|".join(f"{k}:{v}" for k, v in sorted(g.get("deal", {}).items())),
        "undead": any(kinds[k] for k in UNDEAD_KINDS),
        "empire": longest_empire(events, world["year"]) >= empire,
        "quiet_longest": max(b - a for a, b in zip(marks, marks[1:])),
        "wakings": len(sleeps),
        "majors_per_10": 10.0 * sum(majors) / len(majors) if majors else 0.0,
        "breathers": sum(1 for m in majors if m == 0) / len(majors) if majors else 0.0,
        "longest_no_major": longest,
        "lead": sum(lead) / len(lead) if lead else 0.0,
        "long_sleep_major": sum(1 for m in long_sleeps if m) / len(long_sleeps) if long_sleeps else 0.0,
    })
    return out


def summarise_world(per_seed):
    seeds = sorted(per_seed)
    n = len(seeds)
    w = [per_seed[s]["world"] for s in seeds]
    templates = Counter(x["template"] for x in w)
    first = Counter(tuple(map(tuple, per_seed[s]["first_three"])) for s in seeds)
    mean = lambda k: statistics.fmean(x[k] for x in w)
    out = {
        "templates": len(templates),
        "template_top_share": templates.most_common(1)[0][1] / n,
        "template_counts": dict(templates.most_common()),
        "deals": len({x["deal"] for x in w}),
        "first_three_top_share": first.most_common(1)[0][1] / n,
        "quiet_over_300": sum(1 for x in w if x["quiet_longest"] > 300) / n,
        "quiet_longest_median": statistics.median(x["quiet_longest"] for x in w),
        "majors_per_10_wakings": mean("majors_per_10"),
        "breather_share": mean("breathers"),
        "longest_no_major": max(x["longest_no_major"] for x in w),
        "lead_coverage": mean("lead"),
        "long_sleep_major": mean("long_sleep_major"),
    }
    for k in list(IRREVERSIBLE) + ["undead", "empire"]:
        out[k] = sum(1 for x in w if x[k]) / n
    return out


def print_world_table(summaries):
    rows = [s for s in summaries if s.get("world")]
    if not rows:
        return
    cols = ["policy", "tmpl", "top", "deals", "first3", "extinct", "arrived", "undead", "empire", "catacl", "magic",
            "q>300", "maj/10w", "breath", "noMaj", "lead", "50y+maj"]
    table = []
    for s in rows:
        x = s["world"]
        table.append([s["policy"], str(x["templates"]), fmt(x["template_top_share"]), str(x["deals"]), fmt(x["first_three_top_share"]),
                      fmt(x["extinct"]), fmt(x["arrived"]), fmt(x["undead"]), fmt(x["empire"]), fmt(x["cataclysm"]), fmt(x["magic"]),
                      fmt(x["quiet_over_300"]), fmt(x["majors_per_10_wakings"], 1), fmt(x["breather_share"]), str(x["longest_no_major"]),
                      fmt(x["lead_coverage"]), fmt(x["long_sleep_major"])])
    widths = [max(len(c), *(len(r[i]) for r in table)) for i, c in enumerate(cols)]
    print("v0.4 world:")
    print(" | ".join(c.ljust(widths[i]) for i, c in enumerate(cols)))
    print("-+-".join("-" * w for w in widths))
    for r in table:
        print(" | ".join(v.ljust(widths[i]) for i, v in enumerate(r)))
    print()


# ---------------------------------------------------------------- aggregation

def entropy(counter):
    n = sum(counter.values())
    return -sum(c / n * math.log2(c / n) for c in counter.values()) if n else 0.0


def zscores(values):
    vals = [v for v in values if v is not None]
    if len(vals) < 2:
        return [0.0 for _ in values]
    mean = statistics.fmean(vals)
    sd = statistics.pstdev(vals)
    if sd == 0:
        return [0.0 for _ in values]
    return [((v - mean) / sd) if v is not None else 0.0 for v in values]


def outcome_key(o):
    return (o["largest_race"], o["hegemon_race"], o["leader_era"], o["living_factions"])


def summarise_policy(policy, per_seed):
    seeds = sorted(per_seed)
    keys = Counter(outcome_key(per_seed[s]["outcome"]) for s in seeds)
    n = len(seeds)
    rarity = {s: -math.log2(keys[outcome_key(per_seed[s]["outcome"])] / n) for s in seeds}
    causal_available = any(per_seed[s]["causal_depth"] is not None for s in seeds)
    cols = {
        "reversals_per_1000y": [per_seed[s]["reversals_per_1000y"] for s in seeds],
        "pacing_cv": [per_seed[s]["pacing_cv"] for s in seeds],
        "persistence": [per_seed[s]["persistence"] for s in seeds],
        "agency": [per_seed[s]["agency"] for s in seeds],
        "rarity": [rarity[s] for s in seeds],
        "causal_depth": [per_seed[s]["causal_depth"] for s in seeds],
    }
    z = {k: zscores(v) for k, v in cols.items()}
    scores = {}
    for i, s in enumerate(seeds):
        total = 0.0
        for k, w in WEIGHTS.items():
            if k == "agency" and policy == "idle":
                continue
            if k == "causal_depth" and not causal_available:
                continue
            total += w * z[k][i]
        scores[s] = total
        per_seed[s]["rarity"] = rarity[s]
        per_seed[s]["score"] = total

    ranked = sorted(seeds, key=lambda s: (scores[s], s))
    first = Counter(tuple(map(tuple, per_seed[s]["first_three"])) for s in seeds)
    first_kinds = Counter(tuple(k for k, _ in per_seed[s]["first_three"]) for s in seeds)
    race_alive = defaultdict(int)
    for s in seeds:
        for race, alive in per_seed[s]["alive"].items():
            race_alive[race] += 1 if alive else 0
    flags = []
    for race in sorted(race_alive):
        if race_alive[race] == 0:
            flags.append(f"race '{race}' dead at the end in all {n} seeds")
    top_first, top_count = first.most_common(1)[0]
    if top_count / n > 0.5:
        flags.append(f"same first three events (kind,subject) in {100.0 * top_count / n:.0f}% of seeds: {list(top_first)}")
    top_fk, top_fk_count = first_kinds.most_common(1)[0]
    if top_fk_count / n > 0.5:
        flags.append(f"same first three event kinds in {100.0 * top_fk_count / n:.0f}% of seeds: {list(top_fk)}")
    no_rev = [s for s in seeds if sum(per_seed[s]["reversals"].values()) == 0]
    if no_rev:
        flags.append(f"no reversal in {len(no_rev)} seed(s): {no_rev[:20]}{'...' if len(no_rev) > 20 else ''}")

    def mean(k):
        vals = [v for v in cols[k] if v is not None]
        return statistics.fmean(vals) if vals else None

    rev_parts = Counter()
    for s in seeds:
        rev_parts.update(per_seed[s]["reversals"])
    years_total = sum(per_seed[s]["years"] for s in seeds)
    return {
        "policy": policy,
        "seeds": n,
        "outcome_entropy_bits": entropy(keys),
        "outcome_entropy_components_bits": {
            "largest_race": entropy(Counter(per_seed[s]["outcome"]["largest_race"] for s in seeds)),
            "hegemon_race": entropy(Counter(per_seed[s]["outcome"]["hegemon_race"] for s in seeds)),
            "leader_era": entropy(Counter(per_seed[s]["outcome"]["leader_era"] for s in seeds)),
            "living_factions": entropy(Counter(per_seed[s]["outcome"]["living_factions"] for s in seeds)),
        },
        "outcome_distribution": [{"largest_race": k[0], "hegemon_race": k[1], "leader_era": k[2], "living_factions": k[3], "seeds": c}
                                 for k, c in keys.most_common()],
        "distinct_outcomes": len(keys),
        "reversals_per_1000y": mean("reversals_per_1000y"),
        "reversals_per_1000y_by_kind": {k: v / (years_total / 1000.0) if years_total else 0 for k, v in rev_parts.items()},
        "causal_depth": mean("causal_depth") if causal_available else None,
        "pacing_cv": mean("pacing_cv"),
        "persistence": mean("persistence"),
        "agency": mean("agency"),
        "events_mean": statistics.fmean(per_seed[s]["events"] for s in seeds),
        "first_three_top_share": top_count / n,
        "agency_chain": agency_summary([per_seed[s]["agency_chain"] for s in seeds]),
        "death_share": sum(1 for s in seeds if per_seed[s].get("death")) / n,
        "dragon_slain_seeds": sum(1 for s in seeds if per_seed[s]["outcome"]["dragon_slain"]),
        "deaths": deaths_summary([per_seed[s].get("death") for s in seeds]),   # v0.4 (S1)
        "hooks": hooks_summary([per_seed[s].get("hooks") for s in seeds], [per_seed[s].get("growth") for s in seeds]),   # v0.4 hooks (S3)
        "arrivals": arrivals_summary([per_seed[s].get("arrivals") for s in seeds]),
        "sift": sift_summary([per_seed[s].get("sift") for s in seeds], BANK_IDS),   # item 14
        "flatness_flags": flags,
        "most_interesting": [headline(s, per_seed[s]) for s in ranked[::-1][:5]],
        "least_interesting": [headline(s, per_seed[s]) for s in ranked[:5]],
        "ranking": ranked[::-1],
        "weights": WEIGHTS,
        "world": summarise_world(per_seed) if all("world" in per_seed[s] for s in seeds) else None,
    }


def agency_summary(rows):
    tot = Counter()
    for r in rows:
        tot.update(r)

    def share(a, b):
        return tot[a] / tot[b] if tot[b] else 0.0
    return {"hook_dragon_share": share("hooks_dragon", "hooks"), "hook_with_refs_share": share("hooks_with_refs", "hooks"),
            "event_chain_share": share("events_dragon", "events"), "headline_chain_share": share("headlines_dragon", "headlines")}


def band_metrics(summaries, sweep_path):
    m = {}
    for s in summaries:
        p = s["policy"]
        m[f"{p}.outcome_entropy_bits"] = s["outcome_entropy_bits"]
        m[f"{p}.first_three_top_share"] = s["first_three_top_share"]
        m[f"{p}.death_share"] = s["death_share"]
        m[f"{p}.unwarned_deaths"] = s["deaths"]["unwarned"]
        if s.get("hooks"):
            m[f"{p}.hook_coverage"] = s["hooks"]["coverage"]
    if sweep_path and os.path.exists(sweep_path):
        with open(sweep_path) as f:
            for k, v in json.load(f).get("bands", {}).items():
                m[f"sweep.{k}"] = v
    return m


def check_bands(path, metrics):
    with open(path) as f:
        bands = json.load(f)["bands"]
    failed_hard = False
    print(f"{'metric':44s} {'value':>8s} {'band':>14s} {'source':>8s}  result")
    for b in bands:
        v = metrics.get(b["metric"])
        lo, hi = b.get("min"), b.get("max")
        band = f"{'' if lo is None else lo}..{'' if hi is None else hi}"
        if v is None:
            print(f"{b['metric']:44s} {'-':>8s} {band:>14s} {b['source']:>8s}  not measured")
            continue
        ok = (lo is None or v >= lo) and (hi is None or v <= hi)
        result = "pass" if ok else ("FAIL" if b["source"] == "v0.4" else "warn")
        failed_hard |= not ok and b["source"] == "v0.4"
        print(f"{b['metric']:44s} {v:8.3f} {band:>14s} {b['source']:>8s}  {result}")
    return failed_hard


def deaths_summary(deaths):
    """v0.4 (S1): deaths by cause (wounds while raiding, a slaying asleep), median year, slayings the briefing before
    the fatal sleep did not warn of."""
    dead = [d for d in deaths if d]
    years = sorted(d["year"] for d in dead)
    return {"wounds": sum(1 for d in dead if d["cause"] == "wounds"), "slaying": sum(1 for d in dead if d["cause"] == "slaying"),
            "median_year": statistics.median(years) if years else None,
            "unwarned": sum(1 for d in dead if d["cause"] == "slaying" and not d.get("warned", True))}


def arrivals_count(wakings_log, after=200):
    """item 06: visitors waiting at the lair at each waking from `after` years past the hatching."""
    kinds, because, wakings = Counter(), Counter(), 0
    for w in wakings_log:
        if w.get("since_hatch", 0) < after:
            continue
        wakings += 1
        for a in w.get("arrivals", []):
            kinds[a["kind"]] += 1
            because[" + ".join(a["because"])] += 1
    return {"wakings": wakings, "kinds": dict(kinds), "because": dict(because)}


def arrivals_summary(runs):
    runs = [r for r in runs if r]
    wakings = sum(r["wakings"] for r in runs)
    kinds, because = Counter(), Counter()
    for r in runs:
        kinds.update(r["kinds"])
        because.update(r["because"])
    total = sum(kinds.values())
    return {"wakings": wakings, "total": total, "per_waking": total / wakings if wakings else 0.0,
            "by_kind": {k: v / wakings if wakings else 0.0 for k, v in sorted(kinds.items())},
            "top_because": because.most_common(10), "runs_with_none": sum(1 for r in runs if not r["kinds"])}


# ---------------------------------------------------------------- item 14: the sifter's briefings (research-story-sifting.md s7)

def sift_seed(wakings_log, world):
    rows = [w["sift"] for w in wakings_log if w.get("sift")]
    if not rows:
        return None
    shown_open, ended = set(), set()
    heads = []
    for r in rows:
        for l in r["lines"]:
            key, t = l["text_key"], l["thread"]
            if not t:
                continue
            if l["kind"] != "callback" and key.endswith(".open"):
                shown_open.add(t)
            if l["kind"] == "callback" and (".cb.closed" in key or key.endswith(".cb.cut")) and t in shown_open:
                ended.add(t)
        heads.append({"pattern": r["headline_pattern"], "dragon": r["headline_dragon"], "reversals": r["headline_reversals"],
                      "has": any(l["kind"] == "headline" for l in r["lines"])})
    body = [sum(1 for l in r["lines"] if l["kind"] != "headline" and l["tier"] < 5) for r in rows]
    return {
        "wakings": len(rows),
        "heads": heads,
        "shown_open": len(shown_open), "ended": len(ended),
        "names_earlier": sum(1 for r in rows if r["names_earlier"]),
        "lift": [r["mean_shown"] - r["mean_candidate"] for r in rows if r["candidates"]],
        "lift_crowded": [r["mean_shown"] - r["mean_candidate"] for r in rows if r["candidates"] > len(r["lines"])],
        "tier5": sum(r["tier5"] for r in rows), "tier5_shown": sum(r["tier5_shown"] for r in rows),
        "fallbacks": sum(1 for r in rows if r["fallbacks"] > 0),
        "exact": sum(1 for r, b in zip(rows, body) if b == r["budget"]), "over": sum(1 for r, b in zip(rows, body) if b > r["budget"]),
        "lines": sum(body), "budget": sum(r["budget"] for r in rows),
        "completed": sorted({c["text_key"][4:] for c in (world.get("sift") or {}).get("compounds", [])}),
        "pairs": [r["pair"] for r in rows if r.get("pair")][:3],
        "chronicle": {lv: [sum(r["chronicle"][lv][i] for r in rows if r.get("chronicle")) for i in range(3)] for lv in ("0", "1", "2")},
    }


def sift_summary(rows, bank_ids):
    runs = [r for r in rows if r]
    if not runs:
        return None
    heads = [h for r in runs for h in r["heads"] if h["has"]]
    by_pattern = Counter(h["pattern"] for h in heads if h["pattern"])
    seeds = len(runs)
    completion = Counter(p for r in runs for p in r["completed"])
    per_run_max = []
    for r in runs:
        c = Counter(h["pattern"] for h in r["heads"] if h["has"] and h["pattern"])
        n = sum(1 for h in r["heads"] if h["has"])
        per_run_max.append(max(c.values()) / n if c and n else 0.0)
    pattern_heads = [h for h in heads if h["pattern"]]
    shown = sum(r["shown_open"] for r in runs)
    lift = [x for r in runs for x in r["lift"]]
    wakings = sum(r["wakings"] for r in runs)
    return {
        "seeds": seeds, "wakings": wakings, "headlines": len(heads),
        "headline_dragon_share": sum(1 for h in heads if h["dragon"] >= 0.6) / len(heads) if heads else 0.0,
        "shown_ended_share": sum(r["ended"] for r in runs) / shown if shown else 0.0,
        "headline_reversals_mean": statistics.fmean(h["reversals"] for h in pattern_heads) if pattern_heads else 0.0,
        "names_earlier_share": sum(r["names_earlier"] for r in runs) / wakings if wakings else 0.0,
        "selection_lift": statistics.fmean(lift) if lift else 0.0,
        "selection_lift_crowded": statistics.fmean(x for r in runs for x in r["lift_crowded"]) if any(r["lift_crowded"] for r in runs) else 0.0,
        "tier5_shown_share": sum(r["tier5_shown"] for r in runs) / sum(r["tier5"] for r in runs) if sum(r["tier5"] for r in runs) else 1.0,
        "fallback_share": sum(r["fallbacks"] for r in runs) / wakings if wakings else 0.0,
        "pattern_headline_share": {p: c / len(heads) for p, c in by_pattern.most_common()},
        "raw_headline_share": sum(1 for h in heads if not h["pattern"]) / len(heads) if heads else 0.0,
        "largest_pattern_share_in_a_run": max(per_run_max) if per_run_max else 0.0,
        "largest_pattern_share_in_a_run_mean": statistics.fmean(per_run_max) if per_run_max else 0.0,
        "never_headline": [p for p in bank_ids if p not in by_pattern],
        "lines_exact_share": sum(r["exact"] for r in runs) / wakings if wakings else 0.0,
        "lines_over": sum(r["over"] for r in runs),
        "lines_per_budget": sum(r["lines"] for r in runs) / sum(r["budget"] for r in runs) if sum(r["budget"] for r in runs) else 0.0,
        "completion_share": {p: completion[p] / seeds for p in bank_ids},
        "pairs": [p for r in runs for p in r["pairs"]][:15],
        # item 05: of the lines with a cause to tell, the share each chronicle level tells
        "causes_shown_share": {lv: (lambda a, b: b / a if a else 0.0)(sum(r["chronicle"][lv][1] for r in runs), sum(r["chronicle"][lv][2] for r in runs))
                               for lv in ("0", "1", "2")},
    }


def bank_ids(data):
    out = []
    folder = os.path.join(data, "patterns")
    if not os.path.isdir(folder):
        return out
    for f in sorted(os.listdir(folder)):
        if f.endswith(".json") and f != "base_rates.json":
            d = json.load(open(os.path.join(folder, f)))
            out += [p["id"] for p in (d["patterns"] if isinstance(d, dict) else d)]
    return out


def print_sift(s):
    if not s:
        return
    print(f"[{s['policy']}] briefings: {s['wakings']} wakings, {s['headlines']} headlines; D>=0.6 {100 * s['headline_dragon_share']:.0f}% (40-70), "
          f"shown threads ended {100 * s['shown_ended_share']:.0f}% (>=60), headline reversals {s['headline_reversals_mean']:.2f} (1-3), "
          f"names an earlier one {100 * s['names_earlier_share']:.0f}% (30-60), lift {s['selection_lift']:+.2f} (>0.15; {s['selection_lift_crowded']:+.2f} where candidates outnumber lines), "
          f"tier 5 shown {100 * s['tier5_shown_share']:.0f}% (100), fallbacks {100 * s['fallback_share']:.0f}% (<=10), "
          f"top pattern in a run {100 * s['largest_pattern_share_in_a_run']:.0f}% max, {100 * s['largest_pattern_share_in_a_run_mean']:.0f}% mean (<=15), "
          f"lines = L(y) {100 * s['lines_exact_share']:.0f}%, over {s['lines_over']}")
    print(f"[{s['policy']}] headline share: raw events {100 * s['raw_headline_share']:.0f}%, " +
          ", ".join(f"{p} {100 * v:.1f}%" for p, v in s["pattern_headline_share"].items()))
    print(f"[{s['policy']}] never headline: {', '.join(s['never_headline']) or 'none'}")
    print(f"[{s['policy']}] causes shown by chronicle level: " + ", ".join(f"{lv} {100 * v:.0f}%" for lv, v in s["causes_shown_share"].items()) + " (0: 0, 2: >=80)")
    print(f"[{s['policy']}] completion (share of seeds; keep 10-60%): " + ", ".join(f"{p} {100 * v:.0f}%" for p, v in s["completion_share"].items()))


def hooks_summary(hooks, growth):
    """v0.4 hooks (S3): hook coverage (wakings with at least one hook, mean per waking), share by kind, the share the
    policy answered, fights, and the growth points the rewards paid against the total per axis."""
    runs = [h for h in hooks if h]
    if not runs:
        return None
    wakings = sum(h["wakings"] for h in runs)
    offered = sum(h["offered"] for h in runs)
    by_kind = Counter()
    reward = Counter()
    total = Counter()
    for h in runs:
        by_kind.update(h.get("by_kind", {}))
        reward.update(h.get("reward_growth", {}))
    for g in growth:
        if g:
            total.update(g)
    return {
        "wakings": wakings,
        "coverage": sum(h["with_hook"] for h in runs) / wakings if wakings else 0.0,
        "per_waking": offered / wakings if wakings else 0.0,
        "offered": offered,
        "answered_share": sum(h["answered"] for h in runs) / offered if offered else 0.0,
        "by_kind": {k: v / offered for k, v in sorted(by_kind.items())} if offered else {},
        "kinds_seen": len(by_kind),
        "max_kind_share": max(by_kind.values()) / offered if offered else 0.0,
        "fights": sum(h["fights"] for h in runs),
        "fight_wounds_mean": (sum(h["fight_wounds"] for h in runs) / sum(h["fights"] for h in runs)) if sum(h["fights"] for h in runs) else 0.0,
        "reward_growth_share": {k: (reward[k] / total[k] if total[k] else 0.0) for k in sorted(total)},
        "growth_mean": {k: total[k] / len(runs) for k in sorted(total)},
    }


def headline(seed, m):
    o = m["outcome"]
    return {
        "seed": seed,
        "score": round(m["score"], 3),
        "reversals_per_1000y": round(m["reversals_per_1000y"], 2),
        "pacing_cv": round(m["pacing_cv"], 3),
        "persistence": round(m["persistence"], 3),
        "agency": round(m["agency"], 3),
        "rarity_bits": round(m["rarity"], 2),
        "causal_depth": m["causal_depth"],
        "outcome": f"{o['largest_race']}/{o['hegemon_race']}/era{o['leader_era']}/{o['living_factions']}f",
        "events": m["events"],
    }


def fmt(v, digits=2):
    if v is None:
        return "n/a"
    return f"{v:.{digits}f}"


def print_table(summaries):
    cols = ["policy", "seeds", "outcomeH", "distinct", "rev/1000y", "causal", "pacingCV", "persist", "agency", "events", "slain", "hooks/wake", "cover", "answered", "flags"]
    rows = []
    for s in summaries:
        rows.append([s["policy"], str(s["seeds"]), fmt(s["outcome_entropy_bits"]), str(s["distinct_outcomes"]),
                     fmt(s["reversals_per_1000y"]), fmt(s["causal_depth"], 3), fmt(s["pacing_cv"], 3), fmt(s["persistence"], 3),
                     fmt(s["agency"], 3), fmt(s["events_mean"], 0), str(s["dragon_slain_seeds"]),
                     fmt((s.get("hooks") or {}).get("per_waking")), fmt((s.get("hooks") or {}).get("coverage")), fmt((s.get("hooks") or {}).get("answered_share")),
                     str(len(s["flatness_flags"]))])
    widths = [max(len(c), *(len(r[i]) for r in rows)) for i, c in enumerate(cols)]
    line = " | ".join(c.ljust(widths[i]) for i, c in enumerate(cols))
    print(line)
    print("-+-".join("-" * w for w in widths))
    for r in rows:
        print(" | ".join(v.ljust(widths[i]) for i, v in enumerate(r)))
    print()
    print_world_table(summaries)
    for s in summaries:
        print(f"[{s['policy']}] top outcomes (largest race / hegemon race / leader era / living factions):")
        for o in s["outcome_distribution"][:4]:
            print(f"    {o['seeds']:4d}  {o['largest_race']}/{o['hegemon_race']}/era{o['leader_era']}/{o['living_factions']}f")
        print(f"[{s['policy']}] reversals per 1000 y by kind: " + ", ".join(f"{k} {v:.2f}" for k, v in sorted(s['reversals_per_1000y_by_kind'].items())))
        d = s.get("deaths")
        if d:   # v0.4 (S1)
            print(f"[{s['policy']}] dragon deaths: wounds {d['wounds']}, slaying {d['slaying']} (unwarned {d['unwarned']}), median year {fmt(d['median_year'], 0)}")
        a = s.get("agency_chain")
        if a and s["policy"] != "idle":
            print(f"[{s['policy']}] agency: hooks reaching a dragon act {100 * a['hook_dragon_share']:.1f}% (with any refs {100 * a['hook_with_refs_share']:.1f}%), "
                  f"events {100 * a['event_chain_share']:.1f}%, waking headlines {100 * a['headline_chain_share']:.1f}%, Jaccard vs idle {s['agency']:.3f}")
        if s.get("sift"):   # item 14
            print_sift({**s["sift"], "policy": s["policy"]})
        h = s.get("hooks")
        if h:   # v0.4 hooks (S3)
            print(f"[{s['policy']}] hooks: {h['per_waking']:.2f} per waking, coverage {100 * h['coverage']:.1f}%, answered {100 * h['answered_share']:.1f}%, "
                  f"{h['kinds_seen']} kinds (max share {100 * h['max_kind_share']:.0f}%), {h['fights']} fights of {h['fight_wounds_mean']:.1f} wounds")
            print(f"[{s['policy']}] hooks by kind: " + ", ".join(f"{k} {100 * v:.0f}%" for k, v in h["by_kind"].items()))
            print(f"[{s['policy']}] growth per run: " + ", ".join(f"{k} {v:.0f} ({100 * h['reward_growth_share'][k]:.0f}% from hooks)" for k, v in h["growth_mean"].items()))
        ar = s.get("arrivals")
        if ar and ar["wakings"]:   # item 06
            print(f"[{s['policy']}] arrivals: {ar['per_waking']:.2f} per waking after year 200 ({ar['total']} in {ar['wakings']} wakings, {ar['runs_with_none']} runs with none)")
            print(f"[{s['policy']}] arrivals per waking by kind: " + ", ".join(f"{k} {v:.3f}" for k, v in ar["by_kind"].items()))
            for text, n in ar["top_because"]:
                print(f"[{s['policy']}]   because {n:6d}  {text}")
        for label, key in (("most", "most_interesting"), ("least", "least_interesting")):
            print(f"[{s['policy']}] {label} interesting seeds:")
            for h in s[key]:
                print(f"    seed {h['seed']:5d} score {h['score']:+6.2f}  rev/1000y {h['reversals_per_1000y']:5.2f}  pacing {h['pacing_cv']:.2f}  "
                      f"persist {h['persistence']:.2f}  agency {h['agency']:.2f}  rarity {h['rarity_bits']:.1f}b  {h['outcome']}  {h['events']} ev")
        if s["flatness_flags"]:
            print(f"[{s['policy']}] flatness flags:")
            for f in s["flatness_flags"]:
                print(f"    - {f}")
        else:
            print(f"[{s['policy']}] flatness flags: none")
        print()


# ---------------------------------------------------------------- narration

ERA_NAMES = ["bronze", "iron", "castle", "gunpowder", "arcane"]
WAKE_REASONS = {"planned": "as planned", "hunger": "driven by hunger", "dragon_slain": "slain in its lair",
                "slay_attempt_failed": "to slayers at the lair door"}
WAR_END = {"conquest": "ends in conquest", "annihilation": "ends in annihilation", "exhaustion": "peters out in exhaustion",
           "collapse": "ends when one side collapses", "submission": "ends in submission", "dragon_defended": "ends when the dragon intervenes"}
RACE_WORD = {"human": "humans", "dwarf": "dwarves", "elf": "elves", "orc": "orcs", "undead": "undead"}


def pretty_region(rid):
    if not rid:
        return "its lands"
    return "the " + " ".join(w.capitalize() for w in rid.split("_"))


def pct(m):
    return f"{100.0 * m:.0f}%"


class Narrator:
    def __init__(self, races, regions, people=None):
        self.races = races
        self.regions = regions
        self.people = people or {}
        self.seen = set()

    def name(self, fid):
        if not fid:
            return "someone"
        if fid in self.regions:
            return pretty_region(fid)
        if fid == "dragon":
            return "the dragon"
        if fid in self.people:
            return self.people[fid]
        label = fid.capitalize()
        if fid in self.races and fid not in self.seen:
            self.seen.add(fid)
            return f"{label} ({RACE_WORD.get(self.races[fid], self.races[fid])})"
        return label

    def sentence(self, e):
        k, s, o, m, tags = e["kind"], e["subject"], e["object"], e["magnitude"], e["tags"]
        S = self.name(s)
        O = self.name(o)
        tags = [t for t in tags if not t.startswith("successor:")]
        t0 = tags[0] if tags else ""
        T0 = self.name(t0) if t0 in self.races else t0
        era = ERA_NAMES[int(m)] if k == "era_advance" and 0 <= int(m) < len(ERA_NAMES) else str(m)
        table = {
            "sleep_begin": lambda: f"The dragon settles into the lair to sleep {m:.0f} years.",
            "wake": lambda: f"The dragon wakes after {m:.0f} years, {WAKE_REASONS.get(o, o)}.",
            "dragon_atrophied": lambda: f"In its long sleep the dragon wastes a little ({m:.2f}).",
            "dragon_starving": lambda: f"The hoard no longer feeds the sleeper; hunger stands at {m:.2f}.",
            "dragon_raised_lineage": lambda: f"A hostage of {S} grows up in the lair; a dragon-raised line begins.",
            "act_burn": lambda: f"The dragon burns {pretty_region(o)} of {S}; {pct(m)} of its wealth goes up in smoke.",
            "act_take": lambda: f"The dragon carries {pct(m)} of {S}'s wealth off to the hoard.",
            "act_kill": lambda: f"The dragon kills among {S} (magnitude {m:.2f}).",
            "act_show": lambda: f"The dragon shows itself over {S} (magnitude {m:.2f}).",
            "act_defend": lambda: f"The dragon stands over {S} and drives off its attackers.",
            "act_give": lambda: f"The dragon gives {S} gifts worth {pct(m)} of its wealth.",
            "act_take_hostage": lambda: f"The dragon takes a hostage from the court of {S}.",
            "act_release": lambda: f"The dragon releases its hostage to {S}.",
            "act_spell": lambda: f"The dragon casts {s}" + (f" on the {o}" if o else "") + ".",
            "war_started": lambda: (f"{S} rises in civil war against {O}." if t0 == "civil_war" else f"{S} declares war on {O}."),
            "war_ended": lambda: f"The war of {S} against {O} {WAR_END.get(t0, 'ends (' + t0 + ')')}.",
            "coalition_formed": lambda: f"{S} raises a coalition against the hegemon {O}.",
            "coalition_ended": lambda: f"The coalition of {S} against {O} {WAR_END.get(t0, 'ends (' + t0 + ')')}.",
            "joined_war": lambda: f"{S} joins the war on the side of {O}" + (f" ({t0})" if t0 else "") + ".",
            "region_taken": lambda: f"{S} takes {pretty_region(o)} from {T0}.",
            "region_settled": lambda: f"{S} settles the empty {pretty_region(o)[4:]}.",
            "region_abandoned": lambda: f"{S} abandons the scorched {pretty_region(o)[4:]} (burn {m:.2f}).",
            "vassal_sworn": lambda: f"{S}, down to its last land, swears fealty to {O}.",
            "vassal_freed": lambda: f"{S} breaks free of {O}" + (f" ({t0.replace('_', ' ')})" if t0 else "") + ".",
            "vassal_revolt": lambda: f"{S} revolts against its overlord {O} and joins the coalition.",
            "vassal_absorbed": lambda: f"The vassal {S} is absorbed into {O}.",
            "collapse": lambda: (f"{S} collapses into {m:.0f} successor states." if m > 1 else f"{S} collapses; a new house takes its single land."),
            "successor_founded": lambda: f"{S} is founded on {pretty_region(o)[4:]} out of the ruins of {T0}.",
            "faction_destroyed": lambda: f"{S} is wiped out; its people scatter as refugees.",
            "era_advance": lambda: f"{S} enters the {era} age.",
            "slayers_founded": lambda: f"{S} founds an order of dragon slayers.",
            "slay_attempt_failed": lambda: f"The slayers of {S} march on the lair and fail (odds were {m:.2f}).",
            "dragon_slain": lambda: f"The slayers of {S} kill the dragon in its lair.",
            "tribute_paid": lambda: f"{S} sends tribute worth {m:.0f} to the lair.",
            "hegemony": lambda: f"{S} now rules {m:.0f} regions: a hegemony over the valley.",
            "hegemony_ended": lambda: f"The hegemony of {S} ends" + (f", its realm down to {m:.0f} regions." if m > 0 else " with its fall.") + (f" ({t0.replace('_', ' ')})" if t0 else ""),
            "person_died": lambda: f"{S} of {O} dies ({', '.join(t.replace('_', ' ') for t in tags)}).",
            "person_rose": lambda: f"{S} rises in {O} as {t0}.",
            "person_crowned": lambda: f"{S} takes the throne of {O} ({t0.replace('_', ' ')}).",
            "reign_named": lambda: f"The reign of {S} over {O} is remembered as {t0}.",
            "hostage_taken": lambda: f"{S} of {O} comes to the lair as a hostage ({t0}).",
            "hostage_released": lambda: f"{S} goes home to {O} after {m:.0f} years in the lair.",
            "raised_in_lair": lambda: f"{S} grows up in the lair, {m:.0f} years a hostage of the dragon.",
            "offer_fulfilled": lambda: f"The dragon does what {O} of {S} asked; the court's faith deepens.",
            "deceived_dragon": lambda: f"{O} of {S} lied to the dragon, and the dragon acted on it.",
            "offer_declined": lambda: f"The dragon sends {O} of {S} away.",
            "dragon_moulted": lambda: f"The dragon moults and wakes in phase {m:.0f}.",
            "act_demand": lambda: f"The dragon makes a demand of {S} ({o}).",
            "act_accept_offering": lambda: f"The dragon accepts an offering from {S}.",
            "act_destroy_altar": lambda: f"The dragon breaks the altar of {S}.",
            "act_miracle": lambda: f"The dragon works a miracle for {S}.",
            "plague": lambda: f"Plague sweeps through {S}.",
            "bad_century": lambda: f"A bad century of poor harvests begins in {S}.",
            "volcanic_winter": lambda: "Ash darkens the sky over the whole valley: a volcanic winter.",
            "golden_age": lambda: f"A great ruler brings {S} a golden age of {m:.0f} years.",
            "tyrant_rises": lambda: f"A tyrant seizes {S} for {m:.0f} years.",
            "tyrant_died": lambda: f"The tyrant of {S} dies.",
            "succession_crisis": lambda: f"A succession crisis shakes {S}.",
            "warlord_died": lambda: f"The warlord of {S} dies and the horde fractures.",
            "remnant_formed": lambda: (f"Beyond the valley an {s} remnant stirs near {pretty_region(o)}." if t0 == "off_map"
                                       else f"The last {RACE_WORD.get(s, s)} faction is gone; a remnant lingers around {pretty_region(o)}."),
            "race_returned": lambda: f"{S} is founded on {pretty_region(o)[4:]}: the {RACE_WORD.get(t0, t0)} return" + (" by the dragon's sponsorship." if len(tags) > 1 and tags[1] == "sponsored" else " after long absence."),
            "orcs_spawned": lambda: f"Orcs gather on the ruined {pretty_region(o)[4:]}: the horde {S}.",
            "elves_withdrew": lambda: f"The elves of {S} withdraw from {pretty_region(o)[4:]} under pressure from their neighbours.",
            "elves_departed": lambda: f"The elves of {S} leave {pretty_region(o)[4:]} for good.",
            "fading_begins": lambda: f"The elves of {S} begin to fade; human wealth is now {pct(m)} of the valley's.",
            "fading_ends": lambda: f"The fading of {S} ends as human wealth falls to {pct(m)}.",
            "dug_in": lambda: f"{S} digs in deep at {pretty_region(o)[4:]}.",
            "wasteland_formed": lambda: f"{S} turns to wasteland under repeated fire.",
            "wasteland_healed": lambda: f"Green returns to the wasteland of {S}.",
            "oath_sworn": lambda: f"{O} swears an oath to the dragon ({t0}).",
            "oath_expired": lambda: f"The oath {s} with {O} lapses" + (" as the party is gone." if t0 == "party_gone" else "."),
            "oath_breached_by_dragon": lambda: f"The dragon breaks its oath to {O} ({t0}); the court remembers the betrayal.",
            "oath_breached_by_them": lambda: f"{O} breaks its oath to the dragon ({t0}).",
            "demand_refused": lambda: f"{S} refuses the dragon's demand ({o}); dread stands at {m:.2f}.",
            "spell_cast": lambda: f"The dragon's spell {s} falls on {O}" + (f" ({t0})" if t0 else "") + ".",
            "spell_failed": lambda: f"The dragon's spell {o} fails ({t0}).",
            "spell_expired": lambda: f"The spell {s} on {O} fades.",
            "terraformed": lambda: f"The dragon reshapes {S} into {o}.",
            "undead_raised": lambda: f"An undead host {S} rises at {pretty_region(o)} (strength {m:.0f}).",
        }
        fn = table.get(k)
        if fn is None:
            extra = ", ".join(x for x in (o, f"{m:g}" if m else "", " ".join(tags)) if x)
            text = f"Something happens to {S}: {k.replace('_', ' ')}" + (f" ({extra})" if extra else "") + "."
        else:
            text = fn()
        if e.get("count", 1) > 1:
            text += f" (x{e['count']})"
        return text


def narrate(events, world, seed, policy, years):
    """the chronicle by century; an event's `text` (the sim's narrator, with its cause clause) wins over Narrator here"""
    races = faction_races(world)
    regions = region_set(world)
    people = {x["id"]: x["name"] for x in world.get("people", []) + world.get("dead_people", [])}
    nar = Narrator(races, regions, people)
    lines = [f"Chronicle of the valley, seed {seed}, dragon policy '{policy}', {world['year']} years.", ""]
    g = world.get("genesis", {})
    if g.get("template"):
        lines[-1:-1] = [f"Start '{g['template']}', {g['prehistory_years']} years of pre-history, the egg hatched in {g['hatched_year']}."]
    by_century = defaultdict(list)
    for e in events:
        by_century[e["year"] // 100].append(e)
    last = world["year"] // 100 if world["year"] % 100 else world["year"] // 100 - 1
    for c in range(0, max(last, max(by_century) if by_century else 0) + 1):
        evs = by_century.get(c, [])
        kinds = Counter(e["kind"] for e in evs if e["kind"] not in META_KINDS)
        head = f"=== Years {c * 100}-{c * 100 + 99}: {len(evs)} events"
        busiest = [k.replace("_", " ") for k, _ in kinds.most_common(2)]
        if busiest:
            head += " (mostly " + ", ".join(busiest) + ")"
        lines.append(head + " ===")
        if not evs:
            lines.append("  A quiet century; nothing the chroniclers thought worth a line.")
        for e in evs:
            lines.append(f"  {e['year']:5d}  {e.get('text') or nar.sentence(e)}")
        lines.append("")
    return "\n".join(lines)


# ---------------------------------------------------------------- main

def write_read_set(args, summaries, count):
    read_dir = os.path.join(args.out, "read")
    os.makedirs(read_dir, exist_ok=True)
    written = []
    for s in summaries:
        if s["policy"] not in ("idle", "played"):
            continue
        ranking = s["ranking"]
        picks = [("top", i + 1, ranking[i]) for i in range(min(count, len(ranking)))]
        picks += [("bottom", i + 1, ranking[-1 - i]) for i in range(min(count, len(ranking)))]
        for label, rank, seed in picks:
            events, world, _ = simulate(args.bin, args.data, seed, args.years, s["policy"], args.sleep, narrate=True, genesis=args.genesis)
            path = os.path.join(read_dir, f"{s['policy']}_{label}_{rank}.txt")
            with open(path, "w") as f:
                f.write(narrate(events, world, seed, s["policy"], args.years))
            written.append(path)
    return written


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bin", default="build/hoard-sim")
    ap.add_argument("--data", default="data")
    ap.add_argument("--seeds", default="1-100")
    ap.add_argument("--years", type=int, default=3000)
    ap.add_argument("--sleep", type=int, default=100)
    ap.add_argument("--policies", default=None, help=f"default {','.join(DEFAULT_POLICIES)}; with --bands idle,raze_all and --policy")
    ap.add_argument("--out", default="out/interest")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    ap.add_argument("--chronicle", type=int, metavar="SEED", help="print a narrated chronicle for this seed and exit")
    ap.add_argument("--policy", default="idle", help="policy for --chronicle")
    ap.add_argument("--read-set", type=int, default=0, metavar="N",
                    help="after the run, write the N most and N least interesting narrated chronicles for idle and played to <out>/read/")
    ap.add_argument("--read", action="store_true", help="print the table (and --read-set chronicles) from an existing <out>/summary.json")
    ap.add_argument("--genesis", action="store_true", help="generated worlds: pass --genesis to the binary")
    ap.add_argument("--bands", nargs="?", const="data/bands.json", default=None,
                    help="check the bands file (generated worlds, played-style policies; sweep bands from --sweep); exit 1 on a failed v0.4 band")
    ap.add_argument("--sweep", default="out/sweep/summary.json", help="sweep.py summary for the sweep.* bands")
    args = ap.parse_args()
    BANK_IDS[:] = bank_ids(args.data)

    if args.read:
        with open(os.path.join(args.out, "summary.json")) as f:
            summaries = json.load(f)["policies"]
        print_table(summaries)
        if args.read_set:
            for path in write_read_set(args, summaries, args.read_set):
                print("wrote", path)
        if args.bands and check_bands(args.bands, band_metrics(summaries, args.sweep)):
            sys.exit(1)
        return

    if args.chronicle is not None:
        events, world, stats = simulate(args.bin, args.data, args.chronicle, args.years, args.policy, args.sleep, narrate=True, genesis=args.genesis)
        if stats.get("legends"):
            print("Legends of the valley:")
            for l in stats["legends"]:
                print(f"  {l['year']:5d}  {l['text']}")
            print()
        print(narrate(events, world, args.chronicle, args.policy, args.years))
        return

    if args.bands:
        args.genesis = True
        if args.policies is None:
            args.policies = ",".join(dict.fromkeys(["idle", "raze_all", args.policy if args.policy != "idle" else "played"]))
    policies = [p for p in (args.policies or ",".join(DEFAULT_POLICIES)).split(",") if p]
    seeds = list(parse_range(args.seeds))
    per_policy = {p: {} for p in policies}
    jobs = [(args.bin, args.data, s, args.years, args.sleep, policies, args.genesis) for s in seeds]
    done = 0
    with ProcessPoolExecutor(max_workers=args.jobs) as ex:
        futures = [ex.submit(worker, j) for j in jobs]
        for fut in as_completed(futures):
            seed, results = fut.result()
            for p, m in results.items():
                per_policy[p][seed] = m
            done += 1
            if done % 100 == 0 or done == len(seeds):
                sys.stderr.write(f"\r{done}/{len(seeds)} seeds")
    sys.stderr.write("\n")

    summaries = []
    for p in policies:
        summary = summarise_policy(p, per_policy[p])
        summaries.append(summary)
        pdir = os.path.join(args.out, p)
        os.makedirs(pdir, exist_ok=True)
        for seed, m in per_policy[p].items():
            with open(os.path.join(pdir, f"{seed}.json"), "w") as f:
                json.dump(m, f, indent=1, sort_keys=True)
    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "summary.json"), "w") as f:
        json.dump({"years": args.years, "sleep": args.sleep, "seeds": args.seeds, "genesis": args.genesis, "policies": summaries}, f, indent=1)
    print_table(summaries)
    if args.read_set:
        for path in write_read_set(args, summaries, args.read_set):
            print("wrote", path)
    if args.bands and check_bands(args.bands, band_metrics(summaries, args.sweep)):
        sys.exit(1)


if __name__ == "__main__":
    main()

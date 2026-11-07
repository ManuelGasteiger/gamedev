"""Steps 3 and 4 of the scenario validation pipeline (scenario-spec.md section 8), used by tools/scenario_check.py --lab:
satisfiability (interval check of every require clause against the sim's ranges; every act role's type emitted by some
bot policy) and the forced-state test (a lab world built from the roles, hoard-sim --lab, every option played)."""
import json, os, re, subprocess, tempfile

CLAUSE = re.compile(r"^(not )?([a-z][a-z0-9_]*)\.([a-z_]+)\s*(>=|<=|==|!=|>|<)\s*(\S+)$")
ORD = {"none": 0, "oral": 1, "written": 2, "monumental": 3, "forming": 1, "ready": 2, "marching": 3, "false": 0, "true": 1}
TIERS = ["none", "oral", "written", "monumental"]
STAGES = ["none", "forming", "ready", "marching"]
INF = 1e9
RANGES = {  # the values a field can take in the sim
    "faction": {"dread": (0, 1), "awe": (0, 1), "cohesion": (0, 1), "faith_share": (0, 1), "piety": (0, 1), "openness": (0, 1), "militarism": (0, 1),
                "era": (0, 4), "wealth_rank": (1, 30), "portal_progress": (0, 0), "cult_fervor": (0, 0), "memory_tier": (0, 3), "slayer_stage": (0, 3)},
    "person": {"age": (0, 300), "renown": (0, 1), "cunning": (0, 1), "dread": (0, 5), "awe": (0, 5), "debt": (0, 5)},
    "dragon": {"might": (0, INF), "flame": (0, INF), "arcana": (0, INF), "lore": (0, INF), "dominion": (0, INF), "phase": (1, 5),
               "wounds_share": (0, 1), "health": (0, INF), "talents": (0, INF), "hostages": (0, INF), "sources": (0, INF), "stamina": (1, 1)},
    "world": {"magic": (0, 1), "year": (0, INF), "wakings": (0, INF), "ignored_streak": (0, INF), "strongest_era": (0, 4), "chronicle_level": (0, 3)},
    "region": {"burn": (0, 1)},
    "remnant": {"strength": (0, 1)},
}
GATED_TALENTS = ["mortal_guise", "earth_shaker", "patient_mind", "cult_of_the_mountain", "dragon_sight"]
VERB_ACTS = {"sleep", "pick_talent", "ignore", "swear", "receive"}   # acts the verbs and bots log besides the policy templates
BOT_POLICIES = ["raze_all", "rest_only", "protect_one", "priest", "schemer"]


def role_type(role):
    return next(k for k in ("act", "faction", "region", "person", "item", "remnant") if k in role)


def clauses(sc):
    for p in sc.get("require", []):
        for c in p.split(" and "):
            m = CLAUSE.match(c.strip())
            if m:
                yield m.groups()


def num(v):
    if v in ORD:
        return float(ORD[v])
    try:
        return float(v)
    except ValueError:
        return None


def intervals(sc):
    """(scope, field) -> [lo, hi] from the require clauses, and word constraints (scope, field) -> value"""
    types = {r: role_type(d) for r, d in sc["roles"].items()}
    box, words, problems = {}, {}, []
    for neg, scope, field, op, val in clauses(sc):
        kind = scope if scope in ("dragon", "world", "lair") else types.get(scope)
        v = num(val)
        if v is None or kind == "lair":
            if op == "==" and not neg:
                words[(scope, field)] = val
            continue
        lo, hi = RANGES.get(kind, {}).get(field, (0, 1) if val in ("true", "false") else (-INF, INF))
        cur = box.setdefault((scope, field), [lo, hi])
        if neg:
            op = {">=": "<", "<=": ">", ">": "<=", "<": ">=", "==": "!=", "!=": "=="}[op]
        eps = 1e-6
        if op == ">=": cur[0] = max(cur[0], v)
        elif op == ">": cur[0] = max(cur[0], v + eps)
        elif op == "<=": cur[1] = min(cur[1], v)
        elif op == "<": cur[1] = min(cur[1], v - eps)
        elif op == "==": cur[0], cur[1] = max(cur[0], v), min(cur[1], v)
    for k, (lo, hi) in box.items():
        if lo > hi + 1e-9:
            problems.append(f"require on {k[0]}.{k[1]} cannot hold: needs [{lo:g}, {hi:g}]")
    return box, words, problems


def emitted_acts(data, scenarios=()):
    """act types some bot logs: the bot policies' templates, the verbs, and the bank's options (bots answer scenarios)"""
    acts = set(VERB_ACTS) | {o["act"]["type"] for sc in scenarios for o in sc["options"]}
    for name in BOT_POLICIES:
        p = json.load(open(os.path.join(data, "policies", name + ".json")))
        acts |= {a["type"] for a in p.get("acts", [])}
    return acts


def satisfiable(sc, emitted):
    _, _, problems = intervals(sc)
    for r, d in sc["roles"].items():
        if role_type(d) != "act":
            continue
        ts = d["act"]["type"] if isinstance(d["act"]["type"], list) else [d["act"]["type"]]
        if not any(t in emitted for t in ts) and not d.get("optional"):
            problems.append(f"act role {r}: no bot policy emits {ts}")
    return problems


def mid(lo, hi, default):
    if lo <= -INF / 2 and hi >= INF / 2:
        return default
    lo = max(lo, -INF / 2)
    hi = min(hi, lo + 1000) if hi >= INF / 2 else hi
    return round((lo + hi) / 2, 4) if hi < INF / 2 else lo + 1


def ref(v):
    m = re.match(r"^\$([a-z][a-z0-9_]*)", v or "")
    return m.group(1) if m else None


def lab_case(sc):
    """a lab spec (sim/scenarios/lab.cpp) whose world lets the scenario's roles bind"""
    box, words, _ = intervals(sc)
    roles = sc["roles"]
    types = {r: role_type(d) for r, d in roles.items()}
    court_of = {}   # act or person role -> the faction role it belongs to
    courts, persons, acts, relics, remnants = [], [], [], [], []
    for r, d in roles.items():
        if types[r] != "faction":
            continue
        is_ = d["faction"].get("is", "")
        m = re.match(r"owner_of\(\$([a-z_0-9]+)", is_)
        if m:
            court_of[m.group(1)] = r
    for r, d in roles.items():
        if types[r] == "person" and d["person"].get("court", "").startswith("$"):
            court_of.setdefault(r, ref(d["person"]["court"]))
    gates = set(sc.get("arrival", {}).get("needs", []))
    arrival_from = ref(sc.get("arrival", {}).get("from"))
    for r, d in roles.items():
        if types[r] != "faction":
            continue
        f = d["faction"]
        c = {"as": r}
        g = lambda k, dflt=None: box.get((r, k), [None, None])
        if "race" in f: c["race"] = f["race"]
        era = max(f.get("era_min", 0), int(box.get((r, "era"), [0, 4])[0]))
        c["era"] = min(era if era else 2, f.get("era_max", 4))
        for k in ("dread", "awe", "cohesion", "piety", "openness"):
            lo, hi = box.get((r, k), [0, 1])
            if k in f:
                lo, hi = max(lo, f[k][0]), min(hi, f[k][1])
            c[k] = mid(lo, hi, 0.5) if (r, k) in box or k in f else {"dread": 0.2, "awe": 0.3, "cohesion": 0.6, "piety": 0.5, "openness": 0.5}[k]
        lo, hi = box.get((r, "militarism"), [0, 1]); c["militarism"] = mid(lo, hi, 0.5)
        lo, hi = box.get((r, "faith_share"), [0, 1]); c["faith"] = mid(lo, hi, 0.3) if (r, "faith_share") in box else 0.3
        tier = int(box.get((r, "memory_tier"), [1, 3])[0])
        tier = max(tier, TIERS.index(f.get("memory_tier_min", "none")))
        hi_tier = int(box.get((r, "memory_tier"), [0, 3])[1])
        c["memory_tier"] = TIERS[min(max(tier, 1), hi_tier) if hi_tier >= 1 else 0]
        tags = [t for t in f.get("has_tag", [])]
        c["tags"] = tags
        c["region_tags"] = []
        if f.get("has_temple") or words.get((r, "has_temple")) == "true" or box.get((r, "has_temple"), [0])[0] >= 1: c["region_tags"].append("temple")
        if f.get("has_library") or box.get((r, "has_library"), [0])[0] >= 1 or "court.has_library" in gates and r == arrival_from: c["region_tags"].append("library")
        if "walls" in tags: c["region_tags"].append("walls")
        stage = int(box.get((r, "slayer_stage"), [0, 3])[0])
        if f.get("has_slayers") or "slayers" in tags or box.get((r, "has_slayers"), [0])[0] >= 1 or stage or (r == arrival_from and gates & {"court.order_alive", "court.grievance"}):
            c["slayers"] = True
        if f.get("slayer_stage"): c["slayer_stage"] = f["slayer_stage"]
        elif stage: c["slayer_stage"] = STAGES[min(stage, 2)]
        if f.get("knows_prophecy") or box.get((r, "knows_prophecy"), [0])[0] >= 1 or (r == arrival_from and "court.knows_prophecy" in gates): c["knows_prophecy"] = True
        at_war = f.get("at_war")
        if box.get((r, "at_war"), [0, 1])[0] >= 1: at_war = True
        if box.get((r, "at_war"), [0, 1])[1] < 1: at_war = False
        if at_war: c["at_war_with"] = ["*"]
        elif at_war is False: c["peace"] = True
        lo, hi = box.get((r, "wealth_rank"), [1, 30])
        if hi < 30: c["wealth"] = 1e7
        nb = re.match(r"(?:neighbour_of|strongest_army\(neighbours)\(\$?([a-z_0-9]+)", f.get("is", ""))
        if nb and nb.group(1) in roles: c["near"] = nb.group(1)
        if r == arrival_from:
            c["lair_known"] = 1.0
            if "court.awe_min" in gates: c["awe"] = max(c["awe"], 0.2)
            if "court.faith_share_min" in gates: c["faith"] = max(c["faith"], 0.6)
            if "court.dares" in gates: c["dread"] = min(c["dread"], 0.5)
            if "court.lost_region" in gates: c["lost_region_years"] = 5
        courts.append(c)
    # courts named by `near` come after the court they are near
    order = []
    while len(order) < len(courts):
        for c in courts:
            if c in order: continue
            if c.get("near") and c["near"] not in [o["as"] for o in order]: continue
            order.append(c)
    courts = order
    for c in courts:   # a war partner named in the scenario
        if c.get("at_war_with") == ["*"]:
            for o in courts:
                if o is not c and o.get("near") == c["as"] and o.get("at_war_with"):
                    c["at_war_with"] = [o["as"]]
    for r, d in roles.items():
        if types[r] != "person":
            continue
        p = d["person"]
        x = {"as": r, "court": "lair" if p.get("court") == "lair" else ref(p.get("court", "")) or (court_of.get(r) or (courts[0]["as"] if courts else ""))}
        if x["court"] == "lair":
            x["from"] = court_of.get(r) or (courts[0]["as"] if courts else "")
        role = p.get("role") or words.get((r, "role"))
        if role == "envoy": x["envoy"] = True; x["role"] = "sage"
        elif role == "hostage": x["role"] = "heir"
        elif role in ("historian", "consort", "champion", "ward"): x["kept"] = role; x["role"] = "sage"
        elif role: x["role"] = role
        else: x["role"] = "sage"
        if words.get((r, "role")): x["role"] = words[(r, "role")]
        tags = [p["tag"]] if "tag" in p else []
        if "slayer" in tags: x["slayer_rank"] = "slayer"; x["role"] = "hero"
        x["tags"] = tags
        if "trait" in p: x["traits"] = [p["trait"]]
        if "memory_has" in p: x["memory"] = [p["memory_has"]]
        if p.get("knows_dragon"): x["known"] = True
        if p.get("is_lie"): x["is_lie"] = True
        if "cunning" in p: x["cunning"] = mid(p["cunning"][0], p["cunning"][1], 0.5)
        lo, hi = box.get((r, "age"), [0, 300])
        if "age" in p: lo, hi = max(lo, p["age"][0]), min(hi, p["age"][1])
        x["age"] = mid(lo, min(hi, 90), 40) if (r, "age") in box or "age" in p else 40
        lo, hi = box.get((r, "debt"), [0, 5])
        if (r, "debt") in box: x["debt"] = max(lo, 1.5)
        if d.get("sex") in ("m", "f"): x["sex"] = d["sex"]
        persons.append(x)
    if "court.kin_alive" in gates and not any(x["court"] == "lair" and x.get("from") == arrival_from for x in persons):
        persons.append({"as": "_kin", "court": "lair", "from": arrival_from, "role": "heir", "age": 30})
    for r, d in roles.items():
        if types[r] != "act":
            continue
        a = d["act"]
        t = a["type"][0] if isinstance(a["type"], list) else a["type"]
        x = {"as": r, "type": t}
        x["target"] = court_of.get(r) or (ref(a.get("target")) if a.get("target") else (courts[0]["as"] if courts else ""))
        if "structure" in a: x["structure"] = a["structure"]
        if "magnitude" in a: x["magnitude"] = round((a["magnitude"][0] + a["magnitude"][1]) / 2, 3)
        if "age_wakings" in a: x["wakings_ago"] = a["age_wakings"][0]
        if "age_years" in a:
            lo, hi = a["age_years"]
            x["years_ago"] = (lo + hi) // 2
            x["wakings_ago"] = x.get("wakings_ago", max(1, (lo + hi) // 200))
        if "source" in a: x["source"] = a["source"]
        if "answers" in a: x["answers"] = a["answers"]
        payload = {}
        for k in a.get("payload_has", []):
            payload[k] = {"purpose": "spy", "subject": "letters", "who": "army", "clause": "tribute"}.get(k, 40)
        if payload: x["payload"] = payload
        acts.append(x)
    for r, d in roles.items():
        if types[r] == "item":
            i = d["item"]
            relics.append({"as": r, "kind": i.get("kind") if i.get("kind") not in (None, "artifact") else "crown", "agenda": i.get("agenda", ""),
                           "court": courts[0]["as"] if courts else ""})
        if types[r] == "remnant":
            remnants.append({"race": d["remnant"].get("race", "orc"), "strength": 0.5})
    growth = {"lore": 20, "arcana": 20, "dominion": 10, "might": 30, "flame": 10}
    for (scope, field), (lo, hi) in box.items():
        if scope == "dragon" and field in growth:
            growth[field] = max(growth[field], lo + 1 if lo > -INF / 2 else growth[field])
    for o in sc["options"]:
        for neg, scope, field, op, val in [CLAUSE.match(c.strip()).groups() for c in o.get("gate", "").split(" and ") if CLAUSE.match(c.strip())]:
            if scope == "dragon" and field in growth and num(val) is not None and op in (">=", ">"):
                growth[field] = max(growth[field], num(val) + 1)
    dragon = {"growth": growth, "talents": GATED_TALENTS, "magic_pool": 10, "knowledge": 10, "faith_pool": 10,
              "phase": int(max(box.get(("dragon", "phase"), [3])[0], 3))}
    if ("dragon", "wounds_share") in box: dragon["wounds"] = 0
    world = {}
    if ("world", "magic") in box: world["magic"] = mid(*box[("world", "magic")], 0.5)
    if ("world", "prophecy_ending") in words: world["prophecy_ending"] = words[("world", "prophecy_ending")]
    return {"scenario": sc["id"], "courts": courts, "persons": persons, "acts": acts, "relics": relics, "remnants": remnants, "dragon": dragon, "world": world}


def forced_state(scenarios, data, binary, base):
    """runs hoard-sim --lab; one result per scenario"""
    spec = {"base": base, "cases": [lab_case(sc) for sc in scenarios]}
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False) as f:
        json.dump(spec, f)
        path = f.name
    try:
        out = subprocess.run([binary, "--data", data, "--lab", path], capture_output=True, text=True)
    finally:
        os.unlink(path)
    if out.returncode != 0:
        raise RuntimeError(out.stderr.strip())
    return [json.loads(l) for l in out.stdout.splitlines() if l.strip()]


def judge(r):
    """problems of one forced-state result"""
    p = []
    if "error" in r: return [r["error"]]
    if not r.get("bound"): return [f"roles do not bind: {r.get('why')}"]
    if not r.get("era_ok"): p.append(f"era {r.get('era')} outside the scenario's eras (lab world)")
    if r.get("closed_gates"): p.append(f"arrival gates closed: {r['closed_gates']}")
    p += [f"text: {x}" for x in r.get("problems", [])]
    for o in r.get("options", []):
        if not o["ok"]: p.append(f"option {o['id']} ({o['type']}) refused: {o.get('reason')}")
        elif not o["changed"]: p.append(f"option {o['id']} ({o['type']}) accepted but changed nothing")
    return p

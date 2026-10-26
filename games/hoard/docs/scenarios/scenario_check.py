"""Steps 1 and 2 of the scenario validation pipeline (scenario-spec.md s8): schema, then lint.
Usage: python3 scenario_check.py <scenario.schema.json> <act-api.schema.json> <vocab.json> <scenarios.json>..."""
import json, re, sys
import jsonschema

TOKEN = re.compile(r"\{([a-z][a-z0-9_]*)\.([a-z][a-z0-9_]*)\}")
ROLEREF = re.compile(r"^\$([a-z][a-z0-9_]*)(?:\.([a-z][a-z0-9_]*))?$")
CLAUSE = re.compile(r"^(?:not )?(?:bound\(([a-z][a-z0-9_]*)\)|has_tag\(([a-z][a-z0-9_]*),\s*([a-z0-9_:]+)\)|([a-z][a-z0-9_]*)\.([a-z_]+)\s*(>=|<=|==|!=|>|<)\s*(\S+))$")
GLOBALS = {"dragon", "lair", "world"}
DUMMY_ID = {"faction": "f1", "region": "r1", "person": "p1", "item": "i1", "remnant": "m1", "act": "a1"}

def role_type(role):
    return next(k for k in ("act", "faction", "region", "person", "item", "remnant") if k in role)

def check(sc, vocab, act_validator, errors):
    e = lambda msg: errors.append(f"{sc['id']}: {msg}")
    roles = sc["roles"]
    types = {r: role_type(d) for r, d in roles.items()}
    def ref_ok(ref, where):
        m = ROLEREF.match(ref or "")
        if not m: return e(f"{where}: bad role reference {ref!r}")
        if m.group(1) not in roles and m.group(1) != "self": e(f"{where}: unknown role ${m.group(1)}")
    def pred_ok(p, where):
        for clause in p.split(" and "):
            m = CLAUSE.match(clause.strip())
            if not m: e(f"{where}: cannot parse predicate clause {clause!r}"); continue
            role = m.group(1) or m.group(2) or m.group(4)
            if role and role not in roles and role not in GLOBALS: e(f"{where}: predicate names unknown role {role!r}")
            if m.group(4):
                scope = m.group(4); field = m.group(5)
                tname = scope if scope in GLOBALS else types.get(scope)
                if tname and field not in vocab["fields"].get(tname, []): e(f"{where}: field {scope}.{field} not in vocab")
    for name, d in roles.items():
        kind = types[name]
        for key in ("is", "remembers", "court", "owner", "adjacent_to", "target", "after", "before"):
            v = d[kind].get(key)
            if isinstance(v, str) and v.startswith("$"): ref_ok(v.split("(")[0] if "(" not in v else "$" + v.split("$")[1].rstrip(")").split(".")[0], f"role {name}.{key}")
            elif key == "is" and isinstance(v, str):
                for r in re.findall(r"\$([a-z_]+)", v):
                    if r not in roles: e(f"role {name}.is: unknown role ${r}")
        if "choose" in d[kind] and d[kind]["choose"] not in vocab["choosers"]: e(f"role {name}: chooser {d[kind]['choose']!r} not in vocab")
    for p in sc.get("require", []): pred_ok(p, "require")
    for f in sc.get("forbid", []):
        if isinstance(f, str): pred_ok(f, "forbid")
        elif "target" in f["act"]: ref_ok(f["act"]["target"], "forbid.act.target")
    for s in sc.get("salience", []): pred_ok(s["if"], "salience")
    for t in sc["tags"]:
        ns, _, val = t.partition(":")
        if ns == "theme" and val not in vocab["themes"]: e(f"tag theme:{val} not in vocab")
        if ns == "tone" and val not in vocab["tones"]: e(f"tag tone:{val} not in vocab")
        if ns == "ending" and val not in vocab["endings"]: e(f"tag ending:{val} not in vocab")
        if ns == "race" and val not in vocab["races"]: e(f"tag race:{val} not in vocab")
        if ns == "faction": ref_ok(val, "tag faction")
    if "arrival" in sc:
        ref_ok(sc["arrival"]["from"], "arrival.from")
        for g in sc["arrival"]["needs"]:
            if g not in vocab["gates"]: e(f"arrival gate {g!r} not in vocab")
    caps = vocab["text_caps"]; text = sc["text"]
    def words(s): return len(s.split())
    if words(text["headline"]) > caps["headline_words"]: e("headline too long")
    bodies = [text["body"]] + [v["body"] for v in text.get("variants", []) if "body" in v]
    for b in bodies:
        if words(b) > caps["body_words"]: e(f"body too long ({words(b)} words)")
    if "because" in text and words(text["because"]) > caps["because_words"]: e("because-line too long")
    for v in text.get("variants", []): pred_ok(v["if"], "variant")
    for s in [text["headline"], text.get("because", "")] + bodies:
        for role, field in TOKEN.findall(s):
            if role not in roles: e(f"token {{{role}.{field}}} names unknown role"); continue
            if field not in vocab["fields"][types[role]]: e(f"token {{{role}.{field}}}: field not in vocab for {types[role]}")
    ids = [o["id"] for o in sc["options"]]
    if len(ids) != len(set(ids)): e("duplicate option ids")
    gate_free = [o for o in sc["options"] if o["act"]["type"] != "ignore" and "gate" not in o]
    if not gate_free: e("no gate-free option besides ignore")
    for o in sc["options"]:
        if words(o["label"]) > caps["label_words"]: e(f"option {o['id']}: label too long")
        if "gate" in o: pred_ok(o["gate"], f"option {o['id']}.gate")
        act = json.loads(json.dumps(o["act"]))
        for key in ("target", "secondary"):
            if key in act:
                ref_ok(act[key], f"option {o['id']}.{key}")
                m = ROLEREF.match(act[key]); rname = m.group(1) if m else None
                kind = "hook" if rname == "self" else types.get(rname, "faction")
                act[key] = {"kind": kind if kind != "act" else "faction", "id": DUMMY_ID.get(kind, "x1")}
        def bind(v, key=None):
            if isinstance(v, str) and v.startswith("$"):
                if v == "$slider": return 0.5
                return {"kind": "person", "id": "p1"} if key == "about" else "x1"
            if isinstance(v, dict): return {k: bind(x, k) for k, x in v.items()}
            if isinstance(v, list): return [bind(x) for x in v]
            return v
        if "structure" in act: act["secondary"] = {"kind": "structure", "id": act.pop("structure")}
        act = bind(act); act.setdefault("source", "scenario"); act.setdefault("year", 0)
        if "payload" in act and "terms" in act["payload"]: act["payload"]["terms"]["parties"] = ["dragon", "f1"]
        errs = list(act_validator.iter_errors(act))
        if errs: e(f"option {o['id']}: act fails act-api schema: {errs[0].message[:120]}")
    for oid, outs in sc.get("outcomes", {}).items():
        if oid not in ids: e(f"outcome for unknown option {oid!r}")
        for out in outs:
            if "history" in out: ref_ok(out["history"]["person"], f"outcome {oid}.history.person")
            if "suppress" in out and out["suppress"]["tag"].split(":")[1] not in vocab["themes"]: e(f"outcome {oid}: suppress tag not a theme")
    if sc.get("once") == "never" and len(text.get("variants", [])) < 2: e("repeatable scenario needs at least 3 texts (body plus 2 variants)")
    if sc["stakes"] == "must":
        paths = [re.match(r"([a-z_]+\.[a-z_]+)\s*(>=|>)", p) for p in sc.get("require", [])]
        ceil = [re.match(r"([a-z_]+\.[a-z_]+)\s*(<=|<)", p) for p in sc.get("require", [])]
        floors = {m.group(1) for m in paths if m}; ceils = {m.group(1) for m in ceil if m}
        if not floors & ceils: e("must scenario needs a floor and a ceiling on the same path")
    if sc["kind"] == "consequence" and not any(types[r] == "act" for r in roles): e("consequence without an act role")
    read = set(re.findall(r"([a-z_]+\.[a-z_]+)", " ".join(sc.get("require", []))))
    if len(read) < 2: e(f"require reads fewer than two distinct state elements ({len(read)})")

def main(argv):
    sschema, aschema, vocab = (json.load(open(p)) for p in argv[1:4])
    sv = jsonschema.Draft202012Validator(sschema); av = jsonschema.Draft202012Validator(aschema)
    scenarios = [s for p in argv[4:] for s in json.load(open(p))]
    errors = []; ids = [s["id"] for s in scenarios]
    if len(ids) != len(set(ids)): errors.append(f"duplicate ids: {sorted({i for i in ids if ids.count(i) > 1})}")
    for sc in scenarios:
        se = sorted(sv.iter_errors(sc), key=lambda x: x.path)
        if se:
            for x in se[:3]: errors.append(f"{sc.get('id','?')}: schema: {'/'.join(map(str, x.path))}: {x.message[:140]}")
            continue
        check(sc, vocab, av, errors)
    used_themes = {t.split(":")[1] for s in scenarios for t in s["tags"] if t.startswith("theme:")}
    print(f"{len(scenarios)} scenarios, {len(errors)} problems; themes used {len(used_themes)}/{len(vocab['themes'])}; unused: {sorted(set(vocab['themes']) - used_themes)}")
    for x in errors: print("  ", x)
    return 1 if errors else 0

if __name__ == "__main__": sys.exit(main(sys.argv))

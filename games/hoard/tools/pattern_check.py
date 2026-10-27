"""Schema and lint for story patterns. Usage: python3 pattern_check.py docs/patterns/pattern.schema.json docs/patterns/vocab.json data/patterns/bank.json"""
import json, re, sys
import jsonschema

VAR = re.compile(r"\?[a-z][a-z0-9_]*")
KINDS_LEVEL0 = None

def kinds_of(m):
    if "kind" not in m: return [k for a in m.get("any_of", []) for k in kinds_of(a)]
    k = m["kind"]; return k if isinstance(k, list) else [k]

def check(p, vocab, errors):
    e = lambda msg: errors.append(f"{p['id']}: {msg}")
    ids = [c["as"] for c in p["clauses"]]
    if len(ids) != len(set(ids)): e("duplicate clause ids")
    level0 = set(vocab["kinds_level0"])
    bound = set()
    for i, c in enumerate(p["clauses"]):
        for k in kinds_of(c):
            if p["level"] == 1 and k not in level0: e(f"clause {c['as']}: kind {k!r} not a level-0 kind")
            if p["level"] == 2 and not (k in level0 or k.startswith("episode.")): e(f"clause {c['as']}: kind {k!r} unknown")
        for ref_key in ("caused_by", "not_caused_by"):
            if ref_key in c and c[ref_key]["ref"] not in ids[:i]: e(f"clause {c['as']}: {ref_key} refers to a later or unknown clause")
        if "same_thread_as" in c and c["same_thread_as"] not in ids[:i]: e(f"clause {c['as']}: same_thread_as refers to a later clause")
        if i == 0 and "within_years" in c: e("first clause has within_years")
        if i > 0 and "within_years" not in c: e(f"clause {c['as']}: within_years missing (every clause after the first needs one)")
        for a in c.get("any_of", []):
            for k in kinds_of(a):
                if k not in kinds_of({"kind": c["kind"]} if "kind" in c else a): e(f"clause {c['as']}: any_of kind {k!r} missing from the clause's kind list")
        used = set(VAR.findall(json.dumps({k: v for k, v in c.items() if k not in ("bind", "where")})))
        newly = used - bound
        bound |= used | set(c.get("bind", {}).keys())
        for bk, bv in c.get("bind", {}).items():
            if not bv.split(".")[0] in ids[:i + 1]: e(f"clause {c['as']}: bind {bk} reads unknown clause {bv.split('.')[0]!r}")
        for t in c.get("tags_all", []) + c.get("tags_any", []) + c.get("tags_none", []):
            if t not in vocab["tags"] and t != "dragon_blood" and not t.startswith("outcome:"): e(f"clause {c['as']}: tag {t!r} not in vocab")
    for u in p.get("unless", []):
        a, b = u["between"]
        if a not in ids or b not in ids or ids.index(a) >= ids.index(b): e(f"unless between {a},{b}: bad clause order")
        for v in VAR.findall(json.dumps(u)):
            if v not in bound: e(f"unless uses unbound variable {v}")
    for v in p.get("once_per", []):
        if v not in bound: e(f"once_per names unbound variable {v}")
    for key in ("subject", "object"):
        v = p["emit"].get(key)
        if v and v.startswith("?") and v not in bound: e(f"emit.{key} unbound variable {v}")
    if "outcome_by" in p:
        for field in p["outcome_by"]:
            cid = field.split(".")[0]
            if cid not in ids: e(f"outcome_by refers to unknown clause {cid!r}")
            if not (field.endswith(".kind") or field.endswith(".tag")): e(f"outcome_by {field}: only .kind or .tag")
            if field.endswith(".kind"):
                clause = next(c for c in p["clauses"] if c["as"] == cid)
                for k in p["outcome_by"][field]:
                    if k not in kinds_of(clause): e(f"outcome_by {field}: {k!r} not among the clause's kinds")
    if p["emit"]["kind"].startswith("episode.") and p["level"] != 1: e("episode emitted by a level-2 pattern")
    if p["emit"]["kind"].startswith("story.") and p["level"] != 2: e("story emitted by a level-1 pattern")
    if "window_years" in p:
        total = sum(c.get("within_years", 0) for c in p["clauses"])
        if total > p["window_years"]: e(f"clause windows sum to {total} > window_years {p['window_years']}")

def main(argv):
    schema, vocab = json.load(open(argv[1])), json.load(open(argv[2]))
    v = jsonschema.Draft202012Validator(schema); errors = []
    pats = []
    for f in argv[3:]:
        d = json.load(open(f))
        pats += d["patterns"] if isinstance(d, dict) else d
    ids = [x["id"] for x in pats]
    if len(ids) != len(set(ids)): errors.append("duplicate pattern ids")
    for p in pats:
        se = list(v.iter_errors(p))
        if se:
            for x in se[:3]: errors.append(f"{p.get('id','?')}: schema: {'/'.join(map(str, x.path))}: {x.message[:140]}")
            continue
        check(p, vocab, errors)
    emitted = {p["emit"]["kind"] for p in pats}
    print(f"{len(pats)} patterns, {len(errors)} problems; levels: {sum(p['level']==1 for p in pats)} episodes, {sum(p['level']==2 for p in pats)} stories; emits {len(emitted)} kinds")
    for x in errors: print("  ", x)
    return 1 if errors else 0

if __name__ == "__main__": sys.exit(main(sys.argv))

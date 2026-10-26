"""Text lint for scenarios (item 19): every language against the English authoring text, plus forced-state renders
through the core renderer (hoard-sim --render-batch).
Used by tools/scenario_check.py --text DATA --bin HOARD_SIM."""
import json, re, subprocess, tempfile, os
from scenario_strings import strings

ROLE = re.compile(r"\{([a-z][a-z0-9_]*)(?=[.:?}])")
STOP_EN = {"the", "and", "of", "you", "your", "with", "from", "they", "their", "is", "has", "have", "to", "who"}
NAMES = {"person": ["Aldric", "Marta", "Voss", "Weiß", "Hax", "Kerz"], "faction": ["Kell", "Dorun", "Voss", "Ysaril"],
         "region": ["Ashford", "Weiß", "Tannmoor", "Hax"], "remnant": ["the Ash Host"]}
ITEMS = {"de": {"m": "Aschenkelch", "f": "Aschenschale", "n": "Aschenhorn"}, "en": {"m": "Ashen Cup", "f": "Ashen Cup", "n": "Ashen Cup"}}
SIZES = {"en": "a third", "de": "ein Drittel", "pseudo": "a third"}
YEARS = [1, 2, 40, 1200]

def role_type(role):
    return next(k for k in ("act", "faction", "region", "person", "item", "remnant") if k in role)

def outside_tokens(s):
    out, depth = [], 0
    for ch in s:
        if ch == "{": depth += 1
        elif ch == "}": depth -= 1
        elif depth == 0: out.append(ch)
    return "".join(out)

def field_of(key):
    f = key.split(".")
    if f[2] == "variant": return f[4]
    return {"option": "label"}.get(f[2], f[2])

def bound_roles(sc, key):
    """roles the text at key may assume are bound: required roles, plus bound(x) in the variant's if or option's gate"""
    roles = {r for r, d in sc["roles"].items() if not d.get("optional")}
    f = key.split(".")
    cond = ""
    if f[2] == "variant": cond = next(v["if"] for v in sc["text"]["variants"] if v["id"] == f[3])
    if f[2] in ("option", "result", "reveal"): cond = next((o.get("gate", "") for o in sc["options"] if o["id"] == f[3]), "")
    return roles | set(re.findall(r"bound\(([a-z][a-z0-9_]*)\)", cond))

def states(sc):
    persons = [r for r, d in sc["roles"].items() if role_type(d) == "person"]
    out = []
    for i, assign in enumerate(["m", "f", "mf", "fm"]):
        for g in "mfn":
            sexes = {r: assign[j % len(assign)] for j, r in enumerate(persons)}
            out.append((sexes, g, YEARS[(i * 3 + "mfn".index(g)) % len(YEARS)]))
    return out

def ctx(sc, lang, sexes, gender, years):
    roles, n = {}, {k: 0 for k in NAMES}
    regions = NAMES["region"]
    for r, d in sc["roles"].items():
        t = role_type(d)
        if t == "act":
            roles[r] = {"kind": "act", "fields": {"place": regions[0], "size": SIZES.get(lang, SIZES["en"]), "target_name": NAMES["faction"][0]},
                        "numbers": {"years_ago": years, "wakings_ago": 1}}
        elif t == "item":
            roles[r] = {"kind": "item", "name": ITEMS.get(lang, ITEMS["en"])[gender], "gender": gender}
        else:
            pool = NAMES[t]
            e = {"kind": t, "name": pool[n[t] % len(pool)]}
            n[t] += 1
            if t == "person": e.update({"noun": d["noun"], "sex": sexes[r]})
            roles[r] = e
    return {"roles": roles}

def render(binary, data, jobs):
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False, encoding="utf-8") as f:
        json.dump(jobs, f, ensure_ascii=False)
        path = f.name
    try:
        out = subprocess.run([binary, "--data", data, "--render-batch", path], capture_output=True, text=True, encoding="utf-8", check=True).stdout
    finally:
        os.unlink(path)
    return json.loads(out)["renders"]

def lint(scenarios, data, binary, langs, caps, errors):
    """returns render counts per language; appends findings to errors"""
    load = lambda lang, part: json.load(open(os.path.join(data, "text", lang, part + ".json"), encoding="utf-8"))
    en_lex = load("en", "lexicon")["nouns"]
    tables = {lang: load(lang, "scenarios") for lang in langs}
    authored = {k: v for sc in scenarios for k, v in strings(sc)}
    by_key = {k: sc for sc in scenarios for k, _ in strings(sc)}
    if tables["en"] != authored: errors.append("data/text/en/scenarios.json differs from the scenario files; run tools/text/scenario_strings.py")
    for lang in langs:
        t = tables[lang]
        for k in authored:
            if k not in t: errors.append(f"{lang}: missing {k}")
        for k in t:
            if k not in authored: errors.append(f"{lang}: key {k} has no English source")
    for k, en in authored.items():
        sc = by_key[k]
        bound = bound_roles(sc, k)
        lit = outside_tokens(en)
        if re.search(r"\{[a-z_]+\.(years_ago|wakings_ago)\} (years?|wakings?)\b", en): errors.append(f"en {k}: number before a unit word; use :year")
        for r in bound:
            d = sc["roles"].get(r)
            if not d or role_type(d) != "person": continue
            forms = {f["nom"] for f in en_lex.get(d["noun"], {}).values()}
            for form in forms:
                if re.search(rf"\b{re.escape(form)}\b", lit, re.I): errors.append(f"en {k}: bare person noun {form!r} while {r} is bound; use {{{r}.noun:def}}")
        for lang in langs:
            if lang == "en" or k not in tables[lang]: continue
            s = tables[lang][k]
            if set(ROLE.findall(s)) != set(ROLE.findall(en)): errors.append(f"{lang} {k}: roles {sorted(set(ROLE.findall(s)))} differ from English {sorted(set(ROLE.findall(en)))}")
            words = re.findall(r"[A-Za-zÄÖÜäöüß]+", outside_tokens(s))
            if (s == en and len(en.split()) >= 3) or sum(w.lower() in STOP_EN for w in words) >= 2: errors.append(f"{lang} {k}: looks like English left in")
    jobs, meta = [], []
    for k in authored:
        sc = by_key[k]
        for lang in langs:
            if k not in tables[lang]: continue
            for sexes, g, years in states(sc):
                jobs.append({"lang": lang, "key": k, "ctx": ctx(sc, lang, sexes, g, years)})
                meta.append((lang, k))
    renders = render(binary, data, jobs)
    counts = {lang: 0 for lang in langs}
    longest = {}
    for (lang, k), r in zip(meta, renders):
        counts[lang] += 1
        text = r["text"]
        bad = list(r["problems"])
        if "{" in text or "}" in text: bad.append("stray brace")
        if "  " in text: bad.append("double space")
        if re.search(r" [,.;:]", text): bad.append("space before punctuation")
        if text[:1].islower(): bad.append("starts lower-case")
        for b in bad: errors.append(f"{lang} {k}: {b}: {text[:90]}")
        longest[(lang, k)] = max(longest.get((lang, k), 0), len(text))
    for (lang, k), n in longest.items():
        cap = caps.get(lang, {}).get(field_of(k))
        if cap and n > cap: errors.append(f"{lang} {k}: {n} characters rendered, cap {cap}")
    return counts, len(renders)

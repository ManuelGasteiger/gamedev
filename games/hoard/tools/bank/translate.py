"""German pass for scenario text (item 19, for item 11 to call per batch): translate with adaptation, keep every role
token, then lint and render under forced states and send the findings back for one repair.
Usage: python3 tools/bank/translate.py --bin build/hoard-sim [--model claude-sonnet-5-5] [--dry] SCENARIO_FILES...
Writes data/text/de/scenarios.json and .meta.json; raw prompts and replies go to out/translate/. Needs ANTHROPIC_API_KEY
and the anthropic package unless --dry. Each call is paid: ask the owner before the first batch of a round."""
import argparse, datetime, json, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "text"))
from scenario_strings import strings, src_hash
from lint import lint

GRAMMAR = """Tokens: {role.field} or {role.field:mod.mod}. Person fields: name, noun, subj, obj, poss, refl. Modifiers:
determiner def | indef | poss2 (your), case nom | acc | dat | gen, a noun key from the lexicon to call the person something
else ({envoy.noun:liar.def.acc}), possessed gender m | f | n | pl for poss ({x.poss:n.acc}). Items: name (with def/indef and a
case), subj, obj. Numbers: {a.years_ago:year.dat} gives "einem Jahr" / "40 Jahren". Selects: {role.gender?m=...|f=...}.
Plural: {a.years_ago:plural(one=...,other=# ...)}. Never write a person noun as a plain word when the role is in the
string: use {role.noun:...}. Sentence-initial tokens are capitalised by the renderer."""

def load(path, dflt):
    return json.load(open(path, encoding="utf-8")) if os.path.exists(path) else dflt

def prompt(scs, todo, style, gold, nouns):
    parts = ["Translate these Hoard scenario strings from English into German. Adapt freely so the German reads as written in German;",
             "keep every role that the English string names as a token, and use only these tokens.", "", GRAMMAR, "",
             "Noun keys in the German lexicon: " + ", ".join(nouns), "", "House style:", style or "(none yet)", ""]
    if gold:
        parts += ["Examples of approved translations:"] + [f"{k}\n  EN: {v['en']}\n  DE: {v['de']}" for k, v in gold.items()] + [""]
    for sc in scs:
        roles = {r: {k: v for k, v in d.items() if k in ("noun", "sex", "optional")} | {"kind": next(k for k in ("act", "faction", "region", "person", "item", "remnant") if k in d)}
                 for r, d in sc["roles"].items()}
        parts += [f"Scenario {sc['id']}; roles: {json.dumps(roles)}"]
        parts += [f"{k}: {v}" for k, v in strings(sc) if k in todo]
        parts.append("")
    parts.append('Answer with one JSON object, key to German string, and nothing else.')
    return "\n".join(parts)

def ask(client, model, text, log):
    msg = client.messages.create(model=model, max_tokens=8000, messages=[{"role": "user", "content": text}])
    reply = "".join(b.text for b in msg.content if b.type == "text")
    with open(log, "w", encoding="utf-8") as f:
        json.dump({"prompt": text, "reply": reply, "usage": {"in": msg.usage.input_tokens, "out": msg.usage.output_tokens}}, f, ensure_ascii=False, indent=1)
    body = reply[reply.find("{"): reply.rfind("}") + 1]
    return json.loads(body)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", required=True)
    ap.add_argument("--data", default="data")
    ap.add_argument("--model", default="claude-sonnet-5-5")
    ap.add_argument("--all", action="store_true", help="retranslate keys that already have German")
    ap.add_argument("--dry", action="store_true")
    ap.add_argument("files", nargs="+")
    a = ap.parse_args()
    scs = [s for f in a.files for s in json.load(open(f, encoding="utf-8"))]
    de_path = os.path.join(a.data, "text", "de", "scenarios.json")
    de, meta = load(de_path, {}), load(de_path.replace(".json", ".meta.json"), {})
    todo = {k for sc in scs for k, _ in strings(sc) if a.all or k not in de}
    nouns = sorted(load(os.path.join(a.data, "text", "de", "lexicon.json"), {}).get("nouns", {}))
    style = open("docs/style-de.md", encoding="utf-8").read() if os.path.exists("docs/style-de.md") else ""
    gold = load("docs/style-de-gold.json", {})
    text = prompt(scs, todo, style, gold, nouns)
    if a.dry or not todo:
        print(text if todo else "nothing to translate")
        return
    import anthropic
    client = anthropic.Anthropic()
    os.makedirs("out/translate", exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    got = ask(client, a.model, text, f"out/translate/{stamp}-pass.json")
    de.update({k: v for k, v in got.items() if k in todo})
    write(de_path, de, meta, scs, todo, a.model)
    found = []
    lint(scs, a.data, a.bin, ["en", "de"], load("docs/scenarios/vocab.json", {}).get("text_caps_chars", {}), found)
    mine = [f for f in found if any(k in f for k in todo)]
    if mine:
        fix = text + "\n\nYour first answer:\n" + json.dumps({k: de[k] for k in todo}, ensure_ascii=False) + \
              "\n\nThe lint and the renders found:\n" + "\n".join(mine) + "\n\nAnswer again with the whole corrected JSON object."
        got = ask(client, a.model, fix, f"out/translate/{stamp}-repair.json")
        de.update({k: v for k, v in got.items() if k in todo})
        write(de_path, de, meta, scs, todo, a.model)
        found = []
        lint(scs, a.data, a.bin, ["en", "de"], load("docs/scenarios/vocab.json", {}).get("text_caps_chars", {}), found)
        mine = [f for f in found if any(k in f for k in todo)]
    print(f"{len(todo)} strings, {len(mine)} findings left")
    for f in mine: print("  ", f)

def write(path, de, meta, scs, todo, model):
    for sc in scs:
        for k, en in strings(sc):
            if k in todo: meta[k] = {"src": src_hash(en), "by": model, "rev": None}
    order = [k for sc in scs for k, _ in strings(sc)]
    de = {k: de[k] for k in order if k in de} | {k: v for k, v in de.items() if k not in order}
    for p, obj in ((path, de), (path.replace(".json", ".meta.json"), meta)):
        with open(p, "w", encoding="utf-8") as f:
            json.dump(obj, f, ensure_ascii=False, indent=1)
            f.write("\n")

if __name__ == "__main__":
    main()

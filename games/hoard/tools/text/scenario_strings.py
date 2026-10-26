"""The flat string table of scenario files: scn.<id>.<field> keys in authoring order.
Usage: python3 tools/text/scenario_strings.py docs/scenarios/seed-*.json   (writes data/text/en/scenarios.json and .meta)"""
import hashlib, json, sys

def strings(sc):
    p, t = f"scn.{sc['id']}.", sc["text"]
    yield p + "headline", t["headline"]
    yield p + "body", t["body"]
    if "because" in t: yield p + "because", t["because"]
    for v in t.get("variants", []):
        if "headline" in v: yield f"{p}variant.{v['id']}.headline", v["headline"]
        yield f"{p}variant.{v['id']}.body", v["body"]
    for o in sc["options"]: yield f"{p}option.{o['id']}", o["label"]
    for oid, line in t.get("results", {}).items(): yield f"{p}result.{oid}", line
    for oid, outs in sc.get("outcomes", {}).items():
        for o in outs:
            if "reveal_to_player" in o: yield f"{p}reveal.{oid}", o["reveal_to_player"]

def src_hash(s): return hashlib.sha1(s.encode("utf-8")).hexdigest()[:12]

def table(paths):
    out = {}
    for path in paths:
        for sc in json.load(open(path, encoding="utf-8")):
            for k, v in strings(sc): out[k] = v
    return out

def write(path, obj):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(obj, f, ensure_ascii=False, indent=1)
        f.write("\n")

if __name__ == "__main__":
    en = table(sys.argv[1:])
    write("data/text/en/scenarios.json", en)
    write("data/text/en/scenarios.meta.json", {k: {"src": src_hash(v), "by": "hand", "rev": "seed"} for k, v in en.items()})
    print(len(en), "strings")

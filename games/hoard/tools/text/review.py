"""The seed scenarios rendered for a native read: English, then the language with every person male and with every
person female (items m and f). Usage: python3 tools/text/review.py --bin build/hoard-sim --lang de docs/scenarios/seed-*.json > review.md"""
import argparse, json
from lint import ctx, render
from scenario_strings import strings

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", required=True)
    ap.add_argument("--data", default="data")
    ap.add_argument("--lang", default="de")
    ap.add_argument("files", nargs="+")
    a = ap.parse_args()
    scs = [s for f in a.files for s in json.load(open(f, encoding="utf-8"))]
    jobs, rows = [], []
    for sc in scs:
        persons = [r for r, d in sc["roles"].items() if "person" in d]
        for k, _ in strings(sc):
            for lang, sex, g in (("en", "m", "m"), (a.lang, "m", "m"), (a.lang, "f", "f")):
                jobs.append({"lang": lang, "key": k, "ctx": ctx(sc, lang, {r: sex for r in persons}, g, 40)})
            rows.append((sc["id"], k))
    out = render(a.bin, a.data, jobs)
    last = None
    for i, (sid, k) in enumerate(rows):
        en, m, f = (out[3 * i + j]["text"] for j in range(3))
        if sid != last:
            print(f"\n## {sid}\n")
            last = sid
        field = k.split(".", 2)[2]
        print(f"**{field}**  \nEN: {en}  \nm: {m}" + ("" if f == m else f"  \nf: {f}") + "\n")

if __name__ == "__main__":
    main()

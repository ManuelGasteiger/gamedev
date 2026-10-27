#!/usr/bin/env python3
"""Dry run for the sifter's surprise term (research-story-sifting.md section 5): per (pattern, era) the number of
completed matches and how often each property occurs among them, properties that co-occur above 0.8 merged into one,
and magnitude deciles per event kind.

  sift_rates.py --bin build/hoard-sim --data data --seeds 1-300 --jobs 2 --out data/patterns/base_rates.json
"""
import argparse, json, os, subprocess, tempfile
from collections import Counter, defaultdict
from concurrent.futures import ProcessPoolExecutor


def run(args):
    binary, data, seed = args
    with tempfile.TemporaryDirectory() as out:
        subprocess.run([binary, "--data", data, "--seed", str(seed), "--years", "3000", "--genesis", "--policy", "played", "--out", out],
                       check=True, capture_output=True)
        world = json.load(open(os.path.join(out, "world_final.json")))
        chron = json.load(open(os.path.join(out, "chronicle.json")))
    compounds = world.get("sift", {}).get("compounds", [])
    mags = defaultdict(list)
    for e in chron:
        if e["id"] > 0: mags[e["kind"]].append(e["magnitude"])
    return [(c["text_key"][4:], next((t[4:] for t in c["tags"] if t.startswith("era:")), "0"), [t for t in c["tags"] if t.startswith("p:")]) for c in compounds], dict(mags)


def deciles(v):
    v = sorted(v)
    return [v[min(len(v) - 1, (len(v) * k) // 10)] for k in range(1, 10)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default="build/hoard-sim")
    ap.add_argument("--data", default="data")
    ap.add_argument("--seeds", default="1-300")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--out", default="data/patterns/base_rates.json")
    a = ap.parse_args()
    lo, hi = (int(x) for x in a.seeds.split("-"))
    matches, mags = [], defaultdict(list)
    with ProcessPoolExecutor(a.jobs) as ex:
        for ms, mg in ex.map(run, [(a.bin, a.data, s) for s in range(lo, hi + 1)]):
            matches += ms
            for k, v in mg.items(): mags[k] += v
    by_pattern = defaultdict(list)
    for pid, era, props in matches: by_pattern[pid].append(set(props))
    # merge: a property that occurs with another in more than 80 % of the matches holding either is dropped
    merged = {}
    for pid, sets in by_pattern.items():
        count = Counter(p for s in sets for p in s)
        props = sorted(count, key=lambda p: (-count[p], p))
        for i, p in enumerate(props):
            for q in props[:i]:
                if q in merged.get(pid, {}): continue
                both = sum(1 for s in sets if p in s and q in s)
                either = sum(1 for s in sets if p in s or q in s)
                if either and both / either > 0.8:
                    merged.setdefault(pid, {})[p] = q
                    break
    rates = {}
    for pid, era, props in matches:
        r = rates.setdefault(f"{pid}|{era}", {"n": 0})
        r["n"] += 1
        for p in props:
            if p not in merged.get(pid, {}): r[p] = r.get(p, 0) + 1
    out = {"note": f"tools/sift_rates.py over played seeds {a.seeds}: completed matches per pattern and valley era, property counts, merged properties, magnitude deciles",
           "rates": dict(sorted(rates.items())), "merged": merged,
           "magnitudes": {k: deciles(v) for k, v in sorted(mags.items()) if len(v) >= 20 and max(v) > min(v)}}
    json.dump(out, open(a.out, "w"), indent=1, sort_keys=False)
    open(a.out, "a").write("\n")
    print(f"{len(matches)} matches over {hi - lo + 1} seeds, {len(rates)} (pattern, era) cells -> {a.out}")


if __name__ == "__main__":
    main()

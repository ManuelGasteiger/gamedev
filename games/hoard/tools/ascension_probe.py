#!/usr/bin/env python3
"""Item 04b: the Ascension test bot over many seeds.

  ascension_probe.py --bin build/hoard-sim --data data --seeds 1-120 [--free] [--jobs 2] [--out out/ascend]

Per condition: share of runs where it was ever met, longest hold, and the best value of each count it is built
from (courts, races, mean dread); then the parts (courts with awe >= 0.6, with faith >= 0.4, both, Dominion, ...).
"""
import argparse, json, os, subprocess, sys
from collections import Counter
from concurrent.futures import ThreadPoolExecutor


def seeds_of(spec):
    out = []
    for part in spec.split(","):
        a, _, b = part.partition("-")
        out += list(range(int(a), int(b or a) + 1))
    return out


def run(args, seed):
    out = os.path.join(args.out, f"s{seed}")
    os.makedirs(out, exist_ok=True)
    bot = "oracle:ascension:free" if args.free else "oracle:ascension"
    cmd = [args.bin, "--data", args.data, "--seed", str(seed), "--years", str(args.years), "--genesis", "--policy", "played",
           "--bot", bot, "--out", out]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
    with open(os.path.join(out, "ascension.json")) as f:
        return json.load(f)


def pct(xs):
    return f"{100 * sum(xs) / len(xs):5.1f} %"


def dist(vals):
    vals = sorted(vals)
    n = len(vals)
    return f"max {vals[-1]:g}, p90 {vals[int(0.9 * (n - 1))]:g}, median {vals[n // 2]:g}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default="build/hoard-sim")
    ap.add_argument("--data", default="data")
    ap.add_argument("--seeds", default="1-120")
    ap.add_argument("--years", type=int, default=3000)
    ap.add_argument("--free", action="store_true")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--out", default="out/ascend")
    args = ap.parse_args()
    runs = list(ThreadPoolExecutor(args.jobs).map(lambda s: run(args, s), seeds_of(args.seeds)))
    n = len(runs)
    print(f"{n} runs, Ascension bot{' (no stamina budget)' if args.free else ''}, {args.years} years")
    print(f"ready {pct([r['ready'] or r['ended'] == 'ascension' for r in runs])}, slain {pct([r['slain'] for r in runs])}, "
          f"median end year {sorted(r['year'] for r in runs)[n // 2]}")
    print("\ncondition    ever met   longest hold (max)   counts: best value (max, p90, median over runs)")
    for cid in runs[0]["conditions"]:
        met = [r["conditions"][cid]["ever_met"] for r in runs]
        hold = max(r["conditions"][cid]["best_years"] for r in runs)
        print(f"  {cid:10} {pct(met)}   {hold:6d} y")
        keys = sorted({k for r in runs for k in r.get("counts", {}) if k.startswith(cid + ".")})
        for k in keys:
            vals = [r["counts"].get(k, 0) for r in runs]
            if k.endswith("mean_dread_min"):
                vals = [r["counts"][k] for r in runs if k in r["counts"]]
                print(f"      {k:28} lowest {min(vals):.2f} in {len(vals)} runs" if vals else f"      {k:28} never 4 courts")
                continue
            print(f"      {k:28} {dist(vals)}")
    together = Counter(r.get("met_together", 0) for r in runs)
    print("\nmost conditions met at one waking: " + ", ".join(f"{k}: {v} runs" for k, v in sorted(together.items())))
    print(f"most conditions held 150 years at one waking: {dist([r.get('held_together', 0) for r in runs])}")
    best_runs = [r for r in runs if r.get("met_together", 0) == max(together)]
    print("missing at that waking in the best runs: " + ", ".join(f"{k} {v}" for k, v in Counter(m for r in best_runs for m in r.get("missing_then", [])).most_common()))
    print("\nparts (best value per run):")
    for k in sorted(runs[0].get("parts", {})):
        print(f"  {k:20} {dist([r['parts'].get(k, 0) for r in runs])}")


if __name__ == "__main__":
    sys.exit(main())

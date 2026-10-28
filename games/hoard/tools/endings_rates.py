#!/usr/bin/env python3
"""Item 04: endings and prophecy over many played runs.

  endings_rates.py --bin build/hoard-sim --data data --seeds 1-300 [--policy played] [--jobs 2] [--out out/endings]

Per enabled ending: the share of runs where each condition was ever met, its longest hold and best count, and where
the ending was ready. Prophecy cards: the share of runs where each fired within 1000 years of the hatching.
"""
import argparse, json, os, subprocess, sys
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
    cmd = [args.bin, "--data", args.data, "--seed", str(seed), "--years", str(args.years), "--genesis", "--policy", args.policy, "--out", out]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
    with open(os.path.join(out, "stats.json")) as f:
        return json.load(f)["endings"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default="build/hoard-sim")
    ap.add_argument("--data", default="data")
    ap.add_argument("--seeds", default="1-300")
    ap.add_argument("--years", type=int, default=3000)
    ap.add_argument("--policy", default="played")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--out", default="out/endings")
    args = ap.parse_args()
    seeds = seeds_of(args.seeds)
    with ThreadPoolExecutor(args.jobs) as pool:
        runs = list(pool.map(lambda s: run(args, s), seeds))
    n = len(runs)
    print(f"{n} runs, policy {args.policy}, {args.years} years")
    for eid in runs[0]["endings"]:
        ready = sum(r["endings"][eid]["ever_ready"] for r in runs)
        print(f"\n{eid}: ready in {ready}/{n} ({100 * ready / n:.1f} %)")
        print("  condition    ever met   longest hold (median of runs that met it)   best count (max)")
        for cid in runs[0]["endings"][eid]["conditions"]:
            cs = [r["endings"][eid]["conditions"][cid] for r in runs]
            met = [c for c in cs if c["ever_met"]]
            holds = sorted(c["best_years"] for c in met)
            med = holds[len(holds) // 2] if holds else 0
            best = max(c["best_value"] for c in cs)
            print(f"  {cid:12} {100 * len(met) / n:6.1f} %   {med:6d} y (max {max(holds) if holds else 0})   {best:g}")
    print("\nprophecy cards, share of runs firing within 1000 years of the hatching:")
    any_card = 0
    for kind in ["prophecy_written", "prophecy_spread", "prophecy_cult", "prophecy_slayers", "prophecy_schism"]:
        k = sum(1 for r in runs if kind in r["prophecy_cards"] and r["prophecy_cards"][kind] <= 1000)
        ever = sum(1 for r in runs if kind in r["prophecy_cards"])
        print(f"  {kind:18} {100 * k / n:5.1f} % (ever {100 * ever / n:.1f} %)")
    for r in runs:
        if any(k in r["prophecy_cards"] and r["prophecy_cards"][k] <= 1000 for k in ["prophecy_cult", "prophecy_slayers", "prophecy_schism"]):
            any_card += 1
    print(f"  at least one of the three cards: {100 * any_card / n:.1f} %")
    myth = sum(1 for r in runs if r["myth"])
    print(f"\nqualifying Myth deaths: {myth}/{n}")
    known = sorted(r["prophecy_known_by"] for r in runs)
    print(f"courts knowing the prophecy at the end: median {known[n // 2]}, max {known[-1]}")


if __name__ == "__main__":
    sys.exit(main())

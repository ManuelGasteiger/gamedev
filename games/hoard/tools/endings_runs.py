#!/usr/bin/env python3
"""Item 04: inter-run pacing (research-meta-progression.md section 6).

  endings_runs.py --bin build/hoard-sim --data data --chains 30 --runs 12 [--jobs 2] [--out out/runs]

Each chain is one player: runs 1 to N on fresh seeds with one profile folder carried (the amnesiac gets an empty one
every run). Bots: amnesiac, carry, oracle:<ending> for each enabled exit ending. Prints P(ending reached by run N)
and the knowledge leverage P_oracle / P_amnesiac at N = 1. An ending is reached by its exit, the Myth by a
qualifying death.
"""
import argparse, json, os, shutil, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ENDINGS = ["mortality", "myth", "ascension"]


def one_run(args, bot, seed, profile):
    out = os.path.join(args.out, "tmp", f"{bot.replace(':', '_')}_{seed}")
    os.makedirs(out, exist_ok=True)
    cmd = [args.bin, "--data", args.data, "--seed", str(seed), "--years", str(args.years), "--genesis", "--policy", "played",
           "--bot", bot, "--profile", profile, "--out", out]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
    with open(os.path.join(out, "stats.json")) as f:
        e = json.load(f)["endings"]
    shutil.rmtree(out, ignore_errors=True)
    reached = set()
    if e["ended"]:
        reached.add(e["ended"])
    if e["myth"]:
        reached.add("myth")
    return reached


def chain(args, bot, c):
    profile = os.path.join(args.out, "profiles", f"{bot.replace(':', '_')}_{c}")
    shutil.rmtree(profile, ignore_errors=True)
    first = {}
    for n in range(1, args.runs + 1):
        if bot == "amnesiac":
            shutil.rmtree(profile, ignore_errors=True)
        for e in one_run(args, bot, 100000 + 1000 * c + n, profile):
            first.setdefault(e, n)
    return first


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default="build/hoard-sim")
    ap.add_argument("--data", default="data")
    ap.add_argument("--chains", type=int, default=30)
    ap.add_argument("--runs", type=int, default=12)
    ap.add_argument("--years", type=int, default=3000)
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--bots", default="amnesiac,carry,oracle:mortality,oracle:ascension")
    ap.add_argument("--out", default="out/runs")
    args = ap.parse_args()
    bots = args.bots.split(",")
    jobs = [(b, c) for b in bots for c in range(1, args.chains + 1)]
    with ThreadPoolExecutor(args.jobs) as pool:
        results = list(pool.map(lambda bc: (bc[0], chain(args, *bc)), jobs))
    curves = {}
    for b in bots:
        firsts = [r for bb, r in results if bb == b]
        curves[b] = {e: [sum(1 for f in firsts if e in f and f[e] <= n) / len(firsts) for n in range(1, args.runs + 1)] for e in ENDINGS}
        curves[b]["any"] = [sum(1 for f in firsts if f and min(f.values()) <= n) / len(firsts) for n in range(1, args.runs + 1)]
    print(f"P(ending reached by run N), {args.chains} chains per bot")
    print("bot                ending     " + " ".join(f"{n:>5}" for n in range(1, args.runs + 1)))
    for b in bots:
        for e in ENDINGS + ["any"]:
            print(f"{b:18} {e:10} " + " ".join(f"{v:5.2f}" for v in curves[b][e]))
    print("\nknowledge leverage at N = 1 (P_oracle / P_amnesiac):")
    for e in ["mortality", "ascension"]:
        o = curves.get(f"oracle:{e}", {}).get(e, [0])[0]
        a = curves["amnesiac"][e][0]
        print(f"  {e:10} oracle {o:.2f} amnesiac {a:.2f} leverage {'inf' if a == 0 and o > 0 else ('n/a' if a == 0 else f'{o / a:.2f}')}")
    with open(os.path.join(args.out, "curves.json"), "w") as f:
        json.dump(curves, f, indent=1)


if __name__ == "__main__":
    sys.exit(main())

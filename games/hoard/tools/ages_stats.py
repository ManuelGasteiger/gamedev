#!/usr/bin/env python3
"""Aggregate stats.json over a seed range.

  ages_stats.py --bin build/ages --data data --seeds 1-200 --years 3000 [--policy idle] [--out out/stats]
  ages_stats.py --read out/stats          # aggregate existing <dir>/s<seed>/stats.json
"""
import argparse
import json
import os
import statistics
import subprocess
import sys


def parse_range(text):
    if "-" in text:
        a, b = text.split("-", 1)
        return range(int(a), int(b) + 1)
    return range(1, int(text) + 1)


def run_seeds(args):
    dirs = []
    for seed in parse_range(args.seeds):
        out = os.path.join(args.out, f"s{seed}")
        cmd = [args.bin, "--data", args.data, "--seed", str(seed), "--years", str(args.years),
               "--sleep", str(args.sleep), "--out", out]
        if args.policy and args.policy != "idle":
            cmd += ["--policy", args.policy]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            sys.stderr.write(f"seed {seed} failed: {res.stderr}")
            continue
        dirs.append(out)
    return dirs


def load_stats(dirs):
    rows = []
    for d in dirs:
        path = os.path.join(d, "stats.json")
        if os.path.isfile(path):
            with open(path) as f:
                rows.append(json.load(f))
    return rows


def aggregate(rows, era2_by=1500):
    lifetimes = {}
    human_era2 = 0
    empty = 0
    orc_share = []
    slain = 0
    for s in rows:
        for f in s["factions"]:
            lifetimes.setdefault(f["race"], []).append(f["lifetime"])
        eras = s["era_timeline"].get("human", [])
        if len(eras) >= 2 and eras[1] is not None and eras[1] <= era2_by:
            human_era2 += 1
        empty += 1 if s["empty_valley"] else 0
        orc_share.append(s["orc_region_share"])
        slain += 1 if s.get("dragon_slain") else 0
    races = ["human", "dwarf", "elf", "orc"]
    every_race = dwarf_alive = elf_alive = no_dominant = humans_largest = hegemony = hunger = 0
    for s in rows:
        alive = s.get("alive", {})
        rem = s.get("remnants", {})
        if all(alive.get(r) or rem.get(r, 0) > 0.1 for r in races):
            every_race += 1
        dwarf_alive += 1 if alive.get("dwarf") else 0
        elf_alive += 1 if alive.get("elf") else 0
        rr = s.get("race_regions", {})
        total = 12
        if max(rr.values(), default=0) <= 0.7 * total:
            no_dominant += 1
        if rr and max(rr, key=rr.get) == "human" and list(rr.values()).count(rr["human"]) == 1:
            humans_largest += 1
        hegemony += 1 if s.get("hegemonies_fallen", 0) > 0 else 0
        hunger += 1 if s.get("hunger_wakes", 0) > 0 else 0
    n = max(1, len(rows))
    return {
        "seeds": len(rows),
        "median_lifetime": {race: statistics.median(v) for race, v in sorted(lifetimes.items())},
        "faction_count": {race: len(v) for race, v in sorted(lifetimes.items())},
        "human_era2_by_%d" % era2_by: human_era2 / n,
        "empty_valley_share": empty / n,
        "orc_share_mean": statistics.mean(orc_share) if orc_share else 0.0,
        "dragon_slain_share": slain / n,
        "every_race_present": every_race / n,
        "dwarf_alive": dwarf_alive / n,
        "elf_alive": elf_alive / n,
        "no_race_over_70pct": no_dominant / n,
        "humans_largest": humans_largest / n,
        "hegemony_rose_and_fell": hegemony / n,
        "hunger_wake": hunger / n,
    }


def print_table(agg):
    print(f"{'metric':<28}{'value':>10}")
    print(f"{'seeds':<28}{agg['seeds']:>10}")
    for race, med in agg["median_lifetime"].items():
        print(f"{'median lifetime ' + race:<28}{med:>10.0f}   (n={agg['faction_count'][race]})")
    for key in agg:
        if key.startswith("human_era2_by_"):
            print(f"{'humans era 2 by ' + key.split('_')[-1]:<28}{agg[key]:>10.2f}")
    print(f"{'empty valley share':<28}{agg['empty_valley_share']:>10.2f}")
    print(f"{'orc share mean':<28}{agg['orc_share_mean']:>10.3f}")
    print(f"{'dragon slain share':<28}{agg['dragon_slain_share']:>10.2f}")
    bands = [("every race present", "every_race_present", ">= 0.95"), ("dwarves alive", "dwarf_alive", ">= 0.60"),
             ("elves alive", "elf_alive", ">= 0.60"), ("no race > 70% regions", "no_race_over_70pct", ">= 0.85"),
             ("humans largest race", "humans_largest", "0.50..0.90"), ("hegemony rose and fell", "hegemony_rose_and_fell", ">= 0.60"),
             ("woke by hunger", "hunger_wake", "raze_all >= 0.50")]
    for label, key, band in bands:
        print(f"{label:<28}{agg[key]:>10.2f}   {band}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bin", default="build/ages")
    ap.add_argument("--data", default="data")
    ap.add_argument("--seeds", default="1-200")
    ap.add_argument("--years", type=int, default=3000)
    ap.add_argument("--sleep", type=int, default=100)
    ap.add_argument("--policy", default="idle")
    ap.add_argument("--out", default="out/stats")
    ap.add_argument("--read", help="aggregate existing run dirs under this path instead of running")
    ap.add_argument("--json", action="store_true", help="print JSON instead of a table")
    args = ap.parse_args()

    if args.read:
        dirs = [os.path.join(args.read, d) for d in sorted(os.listdir(args.read))]
    else:
        dirs = run_seeds(args)
    agg = aggregate(load_stats(dirs))
    if args.json:
        print(json.dumps(agg, indent=1))
    else:
        print_table(agg)


if __name__ == "__main__":
    main()

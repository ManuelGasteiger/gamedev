#!/usr/bin/env python3
"""Sleep-policy sweep (item 01).

  sweep.py --seeds 300 --years 3000 --jobs 8               # every policy, buckets on --bucket-seeds, perturbation on --perturb-seeds
  sweep.py --recorded run.hrp                              # a human recording through the end-of-run metrics
  sweep.py --selftest                                      # 3 seeds x 300 years, checks the CSV columns and rows

Writes out/sweep/<policy>.csv (seed x century, dragon stocks), <policy>_courts.csv, summary.json, summary.md,
replays/<policy>-<seed>.hrp for played and newcomer.
"""
import argparse
import csv
import json
import math
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from collections import Counter, defaultdict
from concurrent.futures import ProcessPoolExecutor, as_completed

POLICIES = ["rest_only", "moult_only", "long_1000", "raid_then_1000", "played", "idle", "newcomer", "raze_all"]
REPLAYED = {"played", "newcomer"}
LETHALITY = ["played", "long_1000"]
BUCKETS = [1, 10, 30, 100, 300, 1000, 3000]
GROWTH_TIE = 0.02
HOARD = ["gold", "books", "artifacts", "art", "royals", "food", "embers"]
AXES = ["might", "flame", "arcana", "lore", "dominion"]
STOCK_COLUMNS = ["seed", "century", "year", "since_hatch", "phase", "health_max", "wounds", "slain"] + \
    [f"hoard_{k}" for k in HOARD] + [f"growth_{k}" for k in AXES]
COURT_COLUMNS = ["seed", "century", "court", "race", "wealth", "era", "anti_dragon", "dread", "awe", "faith", "cohesion", "regions"]
FIRST_HOUR = 150
HATCHLING = 80
MARKS = [500, 1500, 3000]


def read_jsonl(path):
    if not os.path.exists(path):
        return []
    with open(path) as f:
        return [json.loads(line) for line in f if line.strip()]


def races_of(world):
    races = {f["id"]: f["race"] for f in world["factions"]}
    for g in world.get("graveyard", []):
        races.setdefault(g["id"], g["race"])
    return races


def outcome(world):
    by_race = Counter()
    for f in world["factions"]:
        by_race[f["race"]] += len(f["regions"])
    largest = max(by_race.items(), key=lambda kv: (kv[1], kv[0]))[0] if by_race and max(by_race.values()) > 0 else "none"
    hegemon = next((f["race"] for f in world["factions"] if "hegemon" in f["tags"]), "none")
    return {"largest_race": largest, "hegemon_race": hegemon}


def end_metrics(chron, world, stats):
    hatched = (world.get("genesis") or {}).get("hatched_year", 0)
    death = stats.get("dragon_death")
    races = races_of(world)
    end = {"hatched": hatched, "final_since_hatch": world["year"] - hatched, **outcome(world), "dead": bool(death)}
    if death:
        end.update(death_year=death["year"] - hatched, cause=death["cause"], warned=death.get("warned", True),
                   killer_race=races.get(death.get("court", ""), "none") if death["cause"] == "slaying" else "wounds")
    end["hooks_answered_first_hour"] = sum(1 for e in chron if e["kind"] == "hook_resolved" and e["year"] - hatched <= FIRST_HOUR)
    return end


def century_rows(seed, snaps, years):
    rows, courts = [], []
    for c in range(1, years // 100 + 1):
        upto = [s for s in snaps if s["since_hatch"] <= c * 100]
        if not upto or (snaps[-1]["since_hatch"] < c * 100 and not snaps[-1]["slain"]):
            break
        s = upto[-1] if not snaps[-1]["slain"] or snaps[-1]["since_hatch"] > c * 100 else snaps[-1]
        rows.append([seed, c, s["year"], s["since_hatch"], s["phase"], round(s["health_max"], 3), round(s["wounds"], 3), int(s["slain"])] +
                    [round(s["hoard"].get(k, 0), 3) for k in HOARD] + [round(s["growth"].get(k, 0), 3) for k in AXES])
        for f in s["courts"]:
            courts.append([seed, c, f["id"], f["race"], round(f["wealth"], 3), f["era"], round(f["anti_dragon"], 4),
                           round(f["dread"], 4), round(f["awe"], 4), round(f["faith"], 4), round(f["cohesion"], 4), f["regions"]])
        if s["slain"]:
            break
    return rows, courts


def waking_rows(wakings):
    out = []
    for w in wakings:
        if "listed" not in w:
            continue
        acts = w["listed"]["acts"]
        out.append({"index": w["index"], "since_hatch": w["since_hatch"], "phase": w["phase"], "health": w["health_max"] - w["wounds"],
                    "listed": w["listed"]["total"], "era": max((a["era"] for a in acts), default=-1),
                    "burns": sum(1 for a in acts if a["type"] == "burn"), "towers": w.get("towers", 0), "buckets": w.get("buckets")})
    return out


def run_job(job):
    binary, data, seed, years, policy, mode, out_dir, samples = job
    tmp = tempfile.mkdtemp(prefix="sweep_")
    try:
        run = os.path.join(tmp, "run")
        os.makedirs(run)
        cmd = [binary, "--data", data, "--seed", str(seed), "--years", str(years), "--genesis", "--out", run,
               "--snapshots", os.path.join(run, "snap.jsonl"), "--wakings", os.path.join(run, "wakings.jsonl")]
        if policy != "idle":
            cmd += ["--policy", policy]
        if mode == "buckets":
            cmd += ["--buckets", "--bucket-samples", str(samples)]
        if mode.startswith("perturb_"):
            cmd += ["--perturb", mode[8:]]
        if mode in ("plain", "buckets") and policy in REPLAYED:
            cmd += ["--record", os.path.join(out_dir, "replays", f"{policy}-{seed}.hrp")]
        t0 = time.time()
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            raise RuntimeError(f"seed {seed} {policy} {mode}: {res.stderr.strip()}")
        with open(os.path.join(run, "chronicle.json")) as f:
            chron = json.load(f)
        with open(os.path.join(run, "world_final.json")) as f:
            world = json.load(f)
        with open(os.path.join(run, "stats.json")) as f:
            stats = json.load(f)
        snaps = read_jsonl(os.path.join(run, "snap.jsonl"))
        wakings = read_jsonl(os.path.join(run, "wakings.jsonl"))
        rows, courts = century_rows(seed, snaps, years)
        might100 = [s["growth"].get("might", 0) for s in snaps if s["since_hatch"] <= 100]
        result = {"seed": seed, "policy": policy, "mode": mode, "seconds": time.time() - t0, "end": end_metrics(chron, world, stats),
                  "rows": rows, "courts": courts, "wakings": waking_rows(wakings),
                  "might_100": might100[-1] if might100 else 0.0,
                  "phase_years": {ph: next((x["since_hatch"] for x in snaps if x["phase"] >= ph), None) for ph in (2, 3)},
                  "marks": {m: next(([s["phase"], s["hoard"].get("gold", 0), sum(s["hoard"].values())] for s in reversed(snaps) if s["since_hatch"] <= m), None)
                            for m in MARKS}}
        return result
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def run_cached(job):
    """a finished run is kept in out/cache, so an interrupted sweep resumes (delete out/cache after a change to the sim or data)"""
    binary, data, seed, years, policy, mode, out_dir, samples = job
    path = os.path.join(out_dir, "cache", f"{policy}-{mode}-{seed}.json")
    if os.path.exists(path):
        with open(path) as f:
            r = json.load(f)
        r["marks"] = {int(k): v for k, v in r["marks"].items()}
        r["phase_years"] = {int(k): v for k, v in r["phase_years"].items()}
        return r
    r = run_job(job)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path + ".tmp", "w") as f:
        json.dump(r, f)
    os.replace(path + ".tmp", path)
    return r


# ---------------------------------------------------------------- summaries

def entropy(counter):
    n = sum(counter.values())
    return -sum(c / n * math.log2(c / n) for c in counter.values() if c) if n else 0.0


def pct(x):
    return f"{100 * x:.1f}%"


def growth_per_century(results, lo, hi):
    """mean points per century per axis between centuries lo and hi, over seeds alive at hi"""
    per_axis = defaultdict(list)
    for r in results:
        by_c = {row[1]: row for row in r["rows"]}
        a, b = by_c.get(lo), by_c.get(hi)
        if lo == 0:
            a = [0] * len(STOCK_COLUMNS)
        if a is None or b is None or b[7]:
            continue
        for i, k in enumerate(AXES):
            col = STOCK_COLUMNS.index(f"growth_{k}")
            per_axis[k].append((b[col] - a[col]) / (hi - lo))
    return {k: statistics.fmean(v) if v else None for k, v in per_axis.items()}


def policy_summary(policy, results, years):
    n = len(results)
    ends = [r["end"] for r in results]
    dead = [e for e in ends if e["dead"]]
    marks = {}
    for m in MARKS:
        if m > years:
            continue
        lived = [r["marks"][m] for r in results if r["marks"][m] is not None and not (r["end"]["dead"] and r["end"]["death_year"] <= m)]
        marks[m] = {"alive": len(lived) / n, "phase": dict(sorted(Counter(x[0] for x in lived).items())),
                    "gold": statistics.fmean(x[1] for x in lived) if lived else None,
                    "hoard": statistics.fmean(x[2] for x in lived) if lived else None}
    spans = [(0, 5), (5, 15), (15, 30)]
    first_tower = [next((w["index"] for w in r["wakings"] if w["towers"] > 0), None) for r in results]
    met = [t for t in first_tower if t is not None]
    early = [e for e in dead if e["death_year"] <= FIRST_HOUR]
    keys = Counter((e["dead"], e.get("killer_race", "none"), e["largest_race"], e["hegemon_race"]) for e in ends)
    return {
        "policy": policy, "seeds": n,
        "growth_per_century": {f"{a * 100}-{b * 100}": growth_per_century(results, a, b) for a, b in spans if b * 100 <= years},
        "marks": marks,
        "deaths": {"share": len(dead) / n, "slaying": sum(1 for e in dead if e["cause"] == "slaying"),
                   "wounds": sum(1 for e in dead if e["cause"] == "wounds"), "unwarned": sum(1 for e in dead if not e["warned"]),
                   "killers": dict(Counter(e["killer_race"] for e in dead).most_common()),
                   "median_year": statistics.median(e["death_year"] for e in dead) if dead else None},
        "outcome": {"entropy_bits": entropy(keys), "distinct": len(keys),
                    "largest_race": dict(Counter(e["largest_race"] for e in ends).most_common()),
                    "hegemon_race": dict(Counter(e["hegemon_race"] for e in ends).most_common())},
        "first_hour": {"deaths": [[e["death_year"], e["killer_race"]] for e in sorted(early, key=lambda e: e["death_year"])],
                       "deaths_before_80": sum(1 for e in dead if e["death_year"] < HATCHLING),
                       "first_tower_waking_mean": statistics.fmean(met) if met else None, "first_tower_met_share": len(met) / n,
                       "hooks_answered_mean": statistics.fmean(e["hooks_answered_first_hour"] for e in ends),
                       "might_100_mean": statistics.fmean(r["might_100"] for r in results)},
        "phase_year_median": {f"p{ph}": median_or_none([r["phase_years"][ph] for r in results if r["phase_years"][ph] is not None]) for ph in (2, 3)},
        "seconds": sum(r["seconds"] for r in results),
    }


def bucket_table(results):
    """per wake state the best bucket for survival (lowest P(dead) + 0.5 P(slayer band ready)) and growth (most points);
    ties split; 'decisive' counts only states where the buckets differ"""
    wins = {k: Counter() for k in ("survival", "growth", "growth_tol", "survival_decisive", "growth_decisive", "growth_tol_decisive")}
    states = 0
    decisive = Counter()
    for r in results:
        for w in r["wakings"]:
            b = w["buckets"]
            if not b:
                continue
            states += 1
            # growth_tol: buckets within GROWTH_TIE of the best count as tied (a saturated hoard leaves long sleeps a rounding apart)
            for k, score in (("survival", [-(x["dead"] + 0.5 * x["ready"]) for x in b]), ("growth", [x["growth"] for x in b]),
                             ("growth_tol", [x["growth"] for x in b])):
                top = max(score)
                near = top * (1 - GROWTH_TIE) if k == "growth_tol" else top
                best = [x["years"] for x, v in zip(b, score) if v >= near]
                tied = len(best) == len(b)
                decisive[k] += not tied
                for y in best:
                    wins[k][y] += 1 / len(best)
                    if not tied:
                        wins[k + "_decisive"][y] += 1 / len(best)
    if not states:
        return None
    out = {"states": states, "decisive_survival": decisive["survival"] / states, "decisive_growth": decisive["growth"] / states,
           "decisive_growth_tol": decisive["growth_tol"] / states}
    for k in ("survival", "growth", "growth_tol"):
        out[k] = {y: wins[k][y] / states for y in BUCKETS}
        out[k + "_decisive"] = {y: wins[k + "_decisive"][y] / decisive[k] if decisive[k] else 0.0 for y in BUCKETS}
    return out


def lethality_table(results):
    cells = defaultdict(list)
    for r in results:
        for w in r["wakings"]:
            if w["listed"] <= 0 or w["era"] < 0:
                continue
            cells[(w["era"], w["phase"])].append(w["listed"] / max(1e-9, w["health"]))
    return {f"era{e}_phase{p}": {"n": len(v), "mean": statistics.fmean(v), "max": max(v), "over_1": sum(1 for x in v if x > 1) / len(v)}
            for (e, p), v in sorted(cells.items())}


def ending_class(e):
    return (e["dead"], e.get("killer_race", "none"), e["largest_race"], e["hegemon_race"])


def perturb_table(base, perturbed):
    out = {}
    base_by = {r["seed"]: r for r in base}
    any_flip = set()
    seeds = set()
    for mode, results in perturbed.items():
        flips = applied = 0
        for r in results:
            b = base_by.get(r["seed"])
            if not b:
                continue
            seeds.add(r["seed"])
            first = b["wakings"][0] if b["wakings"] else None
            applied += 1 if mode == "perturb_sleep" or (first and first["burns"] > 0) else 0
            if ending_class(r["end"]) != ending_class(b["end"]):
                flips += 1
                any_flip.add(r["seed"])
        out[mode] = {"seeds": len(results), "applied": applied, "flips": flips, "flip_share": flips / len(results) if results else None}
    out["either_flip_share"] = len(any_flip) / len(seeds) if seeds else None
    return out


def median_or_none(xs):
    return statistics.median(xs) if xs else None


TOP_GROWTH = ["moult_only", "long_1000", "raid_then_1000", "played", "newcomer", "raze_all"]


def top_growth(by):
    """per seed, the policy with the most growth points in its last century row (the run's end, or the death)"""
    totals = defaultdict(dict)
    cols = [STOCK_COLUMNS.index(f"growth_{k}") for k in AXES]
    for p in TOP_GROWTH:
        for r in by.get((p, "run"), []):
            if r["rows"]:
                totals[r["seed"]][p] = sum(r["rows"][-1][c] for c in cols)
    seeds = [t for t in totals.values() if len(t) == len(TOP_GROWTH)]
    wins = Counter(max(t, key=lambda k: (t[k], k)) for t in seeds)
    return {"seeds": len(seeds), "share": {p: wins[p] / len(seeds) for p in TOP_GROWTH} if seeds else {}}


def bands_from(summary):
    b = {}
    bt = summary.get("buckets")
    if bt:
        for k in ("survival", "growth"):   # decisive states: a state where every bucket ties says nothing about the bet
            b[f"{k}_buckets_winning_10pct"] = sum(1 for v in bt[k + "_decisive"].values() if v >= 0.10)
            b[f"{k}_top_bucket_share"] = max(bt[k + "_decisive"].values())
    pt = summary.get("perturbation")
    if pt and pt.get("either_flip_share") is not None:
        b["perturb_flip_share"] = pt["either_flip_share"]
    b["hatchling_deaths_before_80"] = sum(p["first_hour"]["deaths_before_80"] for p in summary["policies"].values())
    return b


def fmt(v, d=1):
    return "-" if v is None else f"{v:.{d}f}"


def markdown(summary):
    years = summary["years"]
    out = [f"seeds {summary['seeds']}, years {years}, genesis worlds, wall {summary['wall_seconds']:.0f} s on {summary['jobs']} jobs", ""]
    pols = summary["policies"]
    marks = [m for m in MARKS if m <= years]
    out.append("| policy | deaths | slaying/wounds | killers | median death | outcome H | " + " | ".join(f"phase @{m} (alive)" for m in marks) +
               " | " + " | ".join(f"hoard @{m}" for m in marks) + " |")
    out.append("|" + "---|" * (6 + 2 * len(marks)))
    for p, s in pols.items():
        d = s["deaths"]
        phase = [", ".join(f"p{k}:{v}" for k, v in s["marks"][m]["phase"].items()) + f" ({pct(s['marks'][m]['alive'])})" for m in marks]
        hoard = [fmt(s["marks"][m]["hoard"], 0) for m in marks]
        out.append(f"| {p} ({s['seeds']}) | {pct(d['share'])} | {d['slaying']}/{d['wounds']} | " + ", ".join(f"{k} {v}" for k, v in d["killers"].items()) +
                   f" | {fmt(d['median_year'], 0)} | {s['outcome']['entropy_bits']:.2f} | " + " | ".join(phase) + " | " + " | ".join(hoard) + " |")
    out += ["", "Growth points per century, by axis (seeds alive at the span's end):", ""]
    spans = list(next(iter(pols.values()))["growth_per_century"])
    out.append("| policy | " + " | ".join(f"{a} {s}" for s in spans for a in AXES) + " |")
    out.append("|" + "---|" * (1 + len(spans) * len(AXES)))
    for p, s in pols.items():
        out.append(f"| {p} | " + " | ".join(fmt(s["growth_per_century"][sp].get(a)) for sp in spans for a in AXES) + " |")
    out += ["", f"First hour (0 to {FIRST_HOUR} years after the hatching):", "",
            "| policy | deaths (year, killer) | deaths < 80 | first tower at waking | tower met | hooks answered | Might @100 |", "|---|---|---|---|---|---|---|"]
    for p, s in pols.items():
        f = s["first_hour"]
        out.append(f"| {p} | {', '.join(f'{y} {k}' for y, k in f['deaths']) or '-'} | {f['deaths_before_80']} | {fmt(f['first_tower_waking_mean'])} | "
                   f"{pct(f['first_tower_met_share'])} | {fmt(f['hooks_answered_mean'], 2)} | {fmt(f['might_100_mean'])} |")
    out += ["", "Phase reached, median year since hatching (first snapshot at or past it):", ""]
    for p, s in pols.items():
        out.append(f"- {p}: phase 2 {fmt(s['phase_year_median']['p2'], 0)}, phase 3 {fmt(s['phase_year_median']['p3'], 0)}")
    tg = summary.get("top_growth")
    if tg and tg["seeds"]:
        out += ["", f"Policy with the most growth points at the end, per seed ({tg['seeds']} seeds): " + ", ".join(f"{k} {pct(v)}" for k, v in sorted(tg["share"].items(), key=lambda kv: -kv[1]))]
    out += ["", "Largest race at the end:", ""]
    for p, s in pols.items():
        out.append(f"- {p}: " + ", ".join(f"{k} {v}" for k, v in s["outcome"]["largest_race"].items()))
    bt = summary.get("buckets")
    if bt:
        out += ["", f"Bucket wins under played ({bt['states']} wake states, {summary['bucket_seeds']} seeds, ties split; "
                f"decisive survival {pct(bt['decisive_survival'])}, growth {pct(bt['decisive_growth'])}):", "",
                "| bucket | " + " | ".join(str(y) for y in BUCKETS) + " |", "|" + "---|" * (1 + len(BUCKETS)),
                "| survival, all states | " + " | ".join(pct(bt["survival"][y]) for y in BUCKETS) + " |",
                "| survival, decisive | " + " | ".join(pct(bt["survival_decisive"][y]) for y in BUCKETS) + " |",
                "| growth, all states | " + " | ".join(pct(bt["growth"][y]) for y in BUCKETS) + " |",
                "| growth, decisive | " + " | ".join(pct(bt["growth_decisive"][y]) for y in BUCKETS) + " |"]
        if "growth_tol_decisive" in bt:
            out.append(f"| growth, decisive, within {pct(GROWTH_TIE)} tied | " + " | ".join(pct(bt["growth_tol_decisive"][y]) for y in BUCKETS) + " |")
    for p, t in summary.get("lethality", {}).items():
        out += ["", f"Lethality (listed wounds of the waking's acts / current health), {p}:", "", "| era x phase | wakings | mean | max | > 1 |", "|---|---|---|---|---|"]
        for k, v in t.items():
            out.append(f"| {k} | {v['n']} | {v['mean']:.3f} | {v['max']:.3f} | {pct(v['over_1'])} |")
    pt = summary.get("perturbation")
    if pt:
        out += ["", "Perturbation (played):", ""]
        for k in ("perturb_burn", "perturb_sleep"):
            if k in pt:
                v = pt[k]
                out.append(f"- {k[8:]}: {v['flips']} of {v['seeds']} flip ({pct(v['flip_share'])}); applied in {v['applied']}")
        if pt.get("either_flip_share") is not None:
            out.append(f"- either: {pct(pt['either_flip_share'])}")
    return "\n".join(out) + "\n"


def write_csvs(out, policy, results):
    with open(os.path.join(out, f"{policy}.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(STOCK_COLUMNS)
        for r in sorted(results, key=lambda r: r["seed"]):
            w.writerows(r["rows"])
    with open(os.path.join(out, f"{policy}_courts.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(COURT_COLUMNS)
        for r in sorted(results, key=lambda r: r["seed"]):
            w.writerows(r["courts"])


def sweep(args):
    t0 = time.time()
    os.makedirs(os.path.join(args.out, "replays"), exist_ok=True)
    policies = [p for p in args.policies.split(",") if p]
    cap = {k: int(v) for k, v in (x.split("=") for x in args.policy_seeds.split(",") if x)}
    jobs = []
    for seed in range(1, args.seeds + 1):
        for p in policies:
            if seed > cap.get(p, args.seeds):
                continue
            mode = "buckets" if p == "played" and seed <= args.bucket_seeds else "plain"
            jobs.append((args.bin, args.data, seed, args.years, p, mode, args.out, args.bucket_samples))
        if "played" in policies and seed <= args.perturb_seeds:
            for m in ("perturb_burn", "perturb_sleep"):
                jobs.append((args.bin, args.data, seed, args.years, "played", m, args.out, args.bucket_samples))
    jobs.sort(key=lambda j: j[5] != "buckets")   # the slow ones first
    by = defaultdict(list)
    done = 0
    with ProcessPoolExecutor(max_workers=args.jobs) as ex:
        for fut in as_completed([ex.submit(run_cached, j) for j in jobs]):
            r = fut.result()
            by[(r["policy"], r["mode"] if r["mode"].startswith("perturb") else "run")].append(r)
            done += 1
            if done % 50 == 0 or done == len(jobs):
                sys.stderr.write(f"\r{done}/{len(jobs)} runs, {time.time() - t0:.0f} s")
    sys.stderr.write("\n")
    summary = {"seeds": args.seeds, "years": args.years, "jobs": args.jobs, "bucket_seeds": min(args.bucket_seeds, args.seeds),
               "bucket_samples": args.bucket_samples, "perturb_seeds": min(args.perturb_seeds, args.seeds), "policy_seeds": cap, "policies": {}}
    for p in policies:
        write_csvs(args.out, p, by[(p, "run")])
        summary["policies"][p] = policy_summary(p, by[(p, "run")], args.years)
    played = by[("played", "run")]
    if played:
        summary["buckets"] = bucket_table(played)
        summary["perturbation"] = perturb_table(played, {m: by[("played", m)] for m in ("perturb_burn", "perturb_sleep") if by[("played", m)]})
    summary["top_growth"] = top_growth(by)
    summary["lethality"] = {p: lethality_table(by[(p, "run")]) for p in LETHALITY if by[(p, "run")]}
    summary["bucket_seconds"] = sum(r["seconds"] for r in played if r["mode"] == "buckets")
    summary["wall_seconds"] = time.time() - t0
    summary["bands"] = bands_from(summary)
    with open(os.path.join(args.out, "summary.json"), "w") as f:
        json.dump(summary, f, indent=1)
    text = markdown(summary)
    with open(os.path.join(args.out, "summary.md"), "w") as f:
        f.write(text)
    print(text)
    return summary


def recorded(args):
    tmp = tempfile.mkdtemp(prefix="sweep_rec_")
    try:
        res = subprocess.run([args.bin, "--data", args.data, "--replay", args.recorded, "--out", tmp], capture_output=True, text=True)
        print(res.stdout.strip())
        if res.returncode != 0:
            sys.exit(res.stderr.strip() or res.returncode)
        with open(os.path.join(tmp, "chronicle.json")) as f:
            chron = json.load(f)
        with open(os.path.join(tmp, "world_final.json")) as f:
            world = json.load(f)
        with open(os.path.join(tmp, "stats.json")) as f:
            stats = json.load(f)
        print(json.dumps({"end": end_metrics(chron, world, stats), "hooks": stats.get("hooks"),
                          "growth": (stats.get("dragon_growth") or {}).get("points")}, indent=1))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def selftest(args):
    args.seeds, args.years, args.bucket_seeds, args.perturb_seeds = 3, 300, 1, 2
    args.out = os.path.join(args.out + "-selftest")
    shutil.rmtree(args.out, ignore_errors=True)
    summary = sweep(args)
    bad = []
    for p in args.policies.split(","):
        with open(os.path.join(args.out, f"{p}.csv")) as f:
            rows = list(csv.reader(f))
        if rows[0] != STOCK_COLUMNS:
            bad.append(f"{p}: columns {rows[0]}")
        per_seed = Counter(r[0] for r in rows[1:])
        for seed in ("1", "2", "3"):
            dead = any(r[0] == seed and r[7] == "1" for r in rows[1:])
            if not 1 <= per_seed[seed] <= 3 or (not dead and per_seed[seed] != 3):
                bad.append(f"{p}: seed {seed} has {per_seed[seed]} rows")
        with open(os.path.join(args.out, f"{p}_courts.csv")) as f:
            if next(csv.reader(f)) != COURT_COLUMNS:
                bad.append(f"{p}: court columns")
    for p in REPLAYED & set(args.policies.split(",")):
        if not os.path.exists(os.path.join(args.out, "replays", f"{p}-1.hrp")):
            bad.append(f"no replay for {p}")
    if not summary.get("buckets"):
        bad.append("no bucket states")
    print("selftest " + ("ok" if not bad else "FAILED\n  " + "\n  ".join(bad)))
    sys.exit(1 if bad else 0)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bin", default="build/hoard-sim")
    ap.add_argument("--data", default="data")
    ap.add_argument("--seeds", type=int, default=300)
    ap.add_argument("--years", type=int, default=3000)
    ap.add_argument("--policies", default=",".join(POLICIES))
    ap.add_argument("--bucket-seeds", type=int, default=300, help="played seeds with the per-waking bucket analysis (the slow part)")
    ap.add_argument("--bucket-samples", type=int, default=2)
    ap.add_argument("--policy-seeds", default="", help="per-policy seed cap, e.g. rest_only=30 (rest_only wakes every 1 to 5 years and is slow)")
    ap.add_argument("--perturb-seeds", type=int, default=100)
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    ap.add_argument("--out", default="out/sweep")
    ap.add_argument("--recorded", metavar="HRP", help="replay a recording and print its end-of-run metrics")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    if args.recorded:
        recorded(args)
    elif args.selftest:
        selftest(args)
    else:
        sweep(args)


if __name__ == "__main__":
    main()

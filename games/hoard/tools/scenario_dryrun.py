"""Steps 5 to 7 of the scenario validation pipeline (scenario-spec.md section 8): the dry run over seeds and the six bot
policies, the cut and flag lists, the bank metrics, and a JSON summary for the authoring pipeline (item 11).

Bots (spec 8.5) and the policy files they run: burner raze_all, sleeper-hoarder rest_only, protector protect_one, priest
priest, schemer schemer, random random. Each answers scenarios by data/bots/scenario_answers.json. rest_only sleeps one
year at a time (3000 wakings a run); the sleeper-hoarder runs it with --min-sleep 100.

Usage: python3 tools/scenario_dryrun.py --bin build/hoard-sim --data data --seeds 1-300 --jobs 2 --out /tmp/dryrun
       python3 tools/scenario_dryrun.py --read /tmp/dryrun/summary.json"""
import argparse, glob, json, os, shutil, statistics, subprocess, sys, tempfile
from collections import Counter, defaultdict
from concurrent.futures import ProcessPoolExecutor, as_completed

BOTS = {"burner": "raze_all", "sleeper": "rest_only", "protector": "protect_one", "priest": "priest", "schemer": "schemer", "random": "random"}
ERAS = ["bronze", "iron", "castles", "gunpowder", "arcane"]


def parse_range(text):
    a, _, b = text.partition("-")
    return range(int(a), int(b or a) + 1)


def run(args):
    binary, data, seed, years, policy = args
    extra = ["--min-sleep", "100"] if policy == "rest_only" else []
    tmp = tempfile.mkdtemp(prefix="scn_dry_")
    try:
        log = os.path.join(tmp, "scn.jsonl")
        cmd = [binary, "--data", data, "--seed", str(seed), "--years", str(years), "--genesis", "--policy", policy,
               "--scenario-bot", policy, "--scenario-log", log, "--out", os.path.join(tmp, "out")] + extra
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            return seed, policy, None, r.stderr.strip()[:200]
        with open(log) as f:
            return seed, policy, [json.loads(l) for l in f if l.strip()], ""
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def load_bank(data):
    bank = {}
    for path in sorted(glob.glob(os.path.join(data, "scenarios", "*.json"))):
        if path.endswith("vocab.json"):
            continue
        for sc in json.load(open(path)):
            bank[sc["id"]] = sc
    return bank


def pct(xs, q):
    if not xs:
        return 0
    xs = sorted(xs)
    return xs[min(len(xs) - 1, int(q * (len(xs) - 1) + 0.5))]


def summarise(bank, vocab, results, runs_total):
    per = {i: {"eligible": 0, "shown": 0, "runs": set(), "positions": [], "cites": 0, "era_wakings": 0, "answered": Counter(), "refused": Counter(),
               "max_per_run": 0} for i in bank}
    era_wakings = Counter()
    depth_after_act = []
    shown_total = 0
    kinds = Counter()
    themes = Counter()
    for (seed, bot), log in results.items():
        per_run = Counter()
        for line in log:
            era = ERAS[min(line.get("era", 0), len(ERAS) - 1)]
            era_wakings[era] += 1
            if line["acted"]:
                depth_after_act.append(line["depth"])
            for i in line["eligible"]:
                per[i]["eligible"] += 1
            for x in line["shown"]:
                p = per[x["id"]]
                p["shown"] += 1
                p["runs"].add((seed, bot))
                p["positions"].append(line["waking"])
                p["cites"] += 1 if x["cites"] else 0
                p["answered"][x["answered"] or "-"] += 1
                per_run[x["id"]] += 1
                shown_total += 1
                kinds[bank[x["id"]]["kind"]] += 1
                for t in bank[x["id"]]["tags"]:
                    if t.startswith("theme:"):
                        themes[t[6:]] += 1
            for r in line.get("refused", []):
                sid = r.split(".", 1)[0]
                if sid in per:
                    per[sid]["refused"][r.split(".", 1)[1]] += 1
        for i, n in per_run.items():
            per[i]["max_per_run"] = max(per[i]["max_per_run"], n)
    rows, dead, hog_runs, hog_share, loose, tight = [], [], [], [], [], []
    for i, sc in sorted(bank.items()):
        p = per[i]
        eras = sc.get("eras") or ERAS
        ew = sum(era_wakings[e] for e in eras)
        elig = p["eligible"] / ew if ew else 0
        runs = len(p["runs"])
        share = p["shown"] / shown_total if shown_total else 0
        row = {"id": i, "kind": sc["kind"], "eligible_rate": elig, "shown": p["shown"], "runs": runs, "run_share": runs / runs_total,
               "showing_share": share, "median_waking": statistics.median(p["positions"]) if p["positions"] else None,
               "cite_share": p["cites"] / p["shown"] if p["shown"] else 0, "max_per_run": p["max_per_run"],
               "answered": dict(p["answered"]), "refused": dict(p["refused"].most_common(5))}
        rows.append(row)
        if runs < 3: dead.append(i)
        if runs > 0.5 * runs_total: hog_runs.append(i)
        if share > 0.02: hog_share.append(i)
        if elig > 0.6: loose.append(i)
        if elig < 0.01: tight.append(i)
    used_themes = set(themes)
    repeat_over = [r["id"] for r in rows if bank[r["id"]].get("once") == "never" and r["max_per_run"] > 3]
    metrics = {
        "runs": runs_total, "wakings": sum(era_wakings.values()), "shown": shown_total,
        "shown_per_waking": shown_total / max(1, sum(era_wakings.values())),
        "consequence_share": kinds["consequence"] / shown_total if shown_total else 0,
        "kinds": dict(kinds),
        "theme_share_max": max(themes.values()) / shown_total if shown_total else 0,
        "theme_top": themes.most_common(5),
        "themes_never_shown": sorted(set(vocab["themes"]) - used_themes),
        "repeatables_over_3_per_run": repeat_over,
        "depth_after_act": {"median": statistics.median(depth_after_act) if depth_after_act else 0, "p5": pct(depth_after_act, 0.05), "wakings": len(depth_after_act)},
    }
    flags = {"dead": dead, "hog_runs": hog_runs, "hog_showings": hog_share, "too_loose": loose, "too_tight": tight}
    return {"scenarios": rows, "metrics": metrics, "flags": flags}


def print_summary(s):
    print(f"{'scenario':30s} {'kind':12s} {'elig%':>6s} {'shown':>6s} {'runs':>5s} {'run%':>5s} {'show%':>6s} {'wake':>5s} {'cite%':>6s}")
    for r in s["scenarios"]:
        mw = "-" if r["median_waking"] is None else f"{r['median_waking']:.0f}"
        print(f"{r['id']:30s} {r['kind']:12s} {100 * r['eligible_rate']:6.2f} {r['shown']:6d} {r['runs']:5d} {100 * r['run_share']:5.1f} "
              f"{100 * r['showing_share']:6.2f} {mw:>5s} {100 * r['cite_share']:6.0f}")
    m = s["metrics"]
    print(f"\nruns {m['runs']}, wakings {m['wakings']}, shown {m['shown']} ({m['shown_per_waking']:.2f} per waking), consequence share {100 * m['consequence_share']:.0f}%")
    print(f"depth after an act: median {m['depth_after_act']['median']}, p5 {m['depth_after_act']['p5']} over {m['depth_after_act']['wakings']} wakings")
    print(f"top theme share {100 * m['theme_share_max']:.1f}% {m['theme_top']}; themes never shown {m['themes_never_shown']}")
    print(f"repeatables shown more than 3 times in a run: {m['repeatables_over_3_per_run']}")
    for k, v in s["flags"].items():
        print(f"{k}: {v}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bin", default="build/hoard-sim")
    ap.add_argument("--data", default="data")
    ap.add_argument("--seeds", default="1-300")
    ap.add_argument("--years", type=int, default=3000)
    ap.add_argument("--bots", default=",".join(BOTS))
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--out", default="out/dryrun")
    ap.add_argument("--read", help="print an existing summary.json")
    a = ap.parse_args()
    if a.read:
        print_summary(json.load(open(a.read)))
        return 0
    bank = load_bank(a.data)
    vocab = json.load(open(os.path.join(a.data, "scenarios", "vocab.json")))
    jobs = [(a.bin, a.data, s, a.years, BOTS[b]) for s in parse_range(a.seeds) for b in a.bots.split(",")]
    results, errors = {}, []
    with ProcessPoolExecutor(max_workers=a.jobs) as ex:
        futs = [ex.submit(run, j) for j in jobs]
        for n, f in enumerate(as_completed(futs), 1):
            seed, policy, log, err = f.result()
            if log is None:
                errors.append(f"seed {seed} {policy}: {err}")
            else:
                results[(seed, policy)] = log
            if n % 60 == 0 or n == len(jobs):
                sys.stderr.write(f"\r{n}/{len(jobs)} runs")
    sys.stderr.write("\n")
    s = summarise(bank, vocab, results, len(results))
    s["errors"] = errors
    s["bank"] = sorted(bank)
    os.makedirs(a.out, exist_ok=True)
    with open(os.path.join(a.out, "summary.json"), "w") as f:
        json.dump(s, f, indent=1)
    print_summary(s)
    for e in errors[:10]:
        print("error:", e)
    return 0


if __name__ == "__main__":
    sys.exit(main())

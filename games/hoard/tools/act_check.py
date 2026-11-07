#!/usr/bin/env python3
"""Validate act logs against docs/act-api.schema.json and count the acts per type (item 09).

  act_check.py LOG.json [LOG.json ...]                                    # act_log.json files the runner wrote
  act_check.py --bin build/hoard-sim --data data --seeds 1-300 --policies played,schemer,priest --jobs 2 [--out out/act_check]

Prints the number of invalid acts (the first few with their error) and the coverage table: act type -> count, with
every schema type listed (0 when it never appeared). Exit 1 when an act is invalid. Needs jsonschema.
"""
import argparse
import collections
import concurrent.futures
import json
import os
import subprocess
import sys

import jsonschema

HERE = os.path.dirname(os.path.abspath(__file__))
SCHEMA = os.path.join(HERE, "..", "docs", "act-api.schema.json")


def seeds_of(text):
    out = []
    for part in text.split(","):
        a, _, b = part.partition("-")
        out += list(range(int(a), int(b or a) + 1))
    return out


def schema_types(schema):
    return [b["allOf"][1]["properties"]["type"]["const"] for b in schema["oneOf"]]


def run_one(args, policy, seed):
    out = os.path.join(args.out, f"{policy}-{seed}")
    os.makedirs(out, exist_ok=True)
    cmd = [args.bin, "--data", args.data, "--seed", str(seed), "--years", str(args.years), "--genesis", "--no-bank",
           "--policy", policy, "--out", out]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
    return policy, os.path.join(out, "act_log.json")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("logs", nargs="*")
    ap.add_argument("--bin")
    ap.add_argument("--data", default="data")
    ap.add_argument("--seeds", default="1-10")
    ap.add_argument("--years", type=int, default=3000)
    ap.add_argument("--policies", default="played,schemer,priest")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--out", default="out/act_check")
    ap.add_argument("--json", help="write the counts here")
    args = ap.parse_args()

    schema = json.load(open(SCHEMA))
    validator = jsonschema.Draft202012Validator(schema)
    logs = [("file", p) for p in args.logs]
    if args.bin:
        jobs = [(pol, s) for pol in args.policies.split(",") for s in seeds_of(args.seeds)]
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as ex:
            logs += list(ex.map(lambda j: run_one(args, *j), jobs))

    counts = collections.Counter()
    by_policy = collections.defaultdict(collections.Counter)
    bad = 0
    shown = 0
    for policy, path in logs:
        for act in json.load(open(path)):
            counts[act["type"]] += 1
            by_policy[policy][act["type"]] += 1
            errors = list(validator.iter_errors(act))
            if errors:
                bad += 1
                if shown < 5:
                    shown += 1
                    print(f"invalid {path} act {act.get('id')}: {json.dumps(act)[:200]}")
                    print(f"  {min(errors, key=lambda e: len(e.message)).message[:200]}")

    print(f"{sum(counts.values())} acts in {len(logs)} logs, {bad} invalid")
    policies = sorted(by_policy)
    print("act | " + " | ".join(policies) + " | all")
    missing = []
    for t in schema_types(schema):
        row = [str(by_policy[p][t]) for p in policies]
        print(f"{t} | " + " | ".join(row) + f" | {counts[t]}")
        if counts[t] == 0:
            missing.append(t)
    extra = sorted(set(counts) - set(schema_types(schema)))
    if extra:
        print("not in the schema: " + ", ".join(extra))
    print("never: " + (", ".join(missing) if missing else "none"))
    if args.json:
        json.dump({"invalid": bad, "counts": dict(counts), "by_policy": {p: dict(c) for p, c in by_policy.items()}, "never": missing},
                  open(args.json, "w"), indent=1)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

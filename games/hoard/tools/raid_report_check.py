#!/usr/bin/env python3
"""Validate RaidReports against docs/raid_report.schema.json, and the acts raid_diff makes from them (item 15).

  raid_report_check.py REPORT.json [...]                                  # schema only; a file may also be {"world":..., "report":...}
  raid_report_check.py --bin build/hoard-sim --data data SCRIPT.json [...]   # run each {"world", "report"} script, validate the logged acts
                                                                             # against docs/act-api.schema.json, print the acts

Exit 1 when a report or an act is invalid. Needs jsonschema.
"""
import argparse
import json
import os
import subprocess
import sys

import jsonschema

HERE = os.path.dirname(os.path.abspath(__file__))
REPORT_SCHEMA = os.path.join(HERE, "..", "docs", "raid_report.schema.json")
ACT_SCHEMA = os.path.join(HERE, "..", "docs", "act-api.schema.json")


def load(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--bin")
    ap.add_argument("--data", default="data")
    args = ap.parse_args()
    report_validator = jsonschema.Draft202012Validator(load(REPORT_SCHEMA))
    act_validator = jsonschema.Draft202012Validator(load(ACT_SCHEMA))
    bad = 0
    for path in args.files:
        doc = load(path)
        report = doc.get("report", doc) if isinstance(doc, dict) else doc
        errors = sorted(report_validator.iter_errors(report), key=lambda e: list(e.path))
        for e in errors[:5]:
            print(f"{path}: report: {'/'.join(map(str, e.path))}: {e.message}")
        bad += len(errors)
        if not args.bin or errors:
            if not errors:
                print(f"{path}: report ok")
            continue
        run = subprocess.run([args.bin, "--data", args.data, "--raid-report", path], capture_output=True, text=True)
        if run.returncode != 0:
            print(f"{path}: runner failed: {run.stderr.strip()}")
            bad += 1
            continue
        out = json.loads(run.stdout)
        act_errors = 0
        for act in out["acts"]:
            for e in list(act_validator.iter_errors(act))[:1]:
                print(f"{path}: act {act.get('type')}: {e.message}")
                act_errors += 1
        bad += act_errors
        print(f"{path}: {len(out['acts'])} acts, {act_errors} invalid, {len(out['refused'])} refused, warnings {out['warnings']}")
        for act in out["acts"]:
            print("  ", json.dumps({k: act[k] for k in ("type", "magnitude", "target", "secondary", "payload") if k in act}))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()

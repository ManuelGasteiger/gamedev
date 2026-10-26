#!/usr/bin/env python3
"""first_div.py a.jsonl b.jsonl: the first checkpoint where two state-hash traces differ, and which parts."""
import json
import sys


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    with open(sys.argv[1]) as fa, open(sys.argv[2]) as fb:
        for n, (la, lb) in enumerate(zip(fa, fb)):
            a, b = json.loads(la), json.loads(lb)
            if a["root"] == b["root"]:
                continue
            parts = sorted(k for k in set(a["parts"]) | set(b["parts"]) if a["parts"].get(k) != b["parts"].get(k))
            print(f"first divergence at checkpoint {n}, year {a['year']} (other trace: {b['year']})")
            print("parts: " + ", ".join(parts))
            return 1
    print("traces agree on every common checkpoint")
    return 0


if __name__ == "__main__":
    sys.exit(main())

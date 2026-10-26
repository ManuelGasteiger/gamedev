#!/usr/bin/env python3
"""chronicle_diff.py old.txt new.txt: the first changed line of two text goldens, ten lines of context,
and the events added and removed by kind."""
import collections
import sys


def kinds(lines):
    return collections.Counter(l.split()[1] for l in lines if len(l.split()) > 1)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    a = open(sys.argv[1]).read().splitlines()
    b = open(sys.argv[2]).read().splitlines()
    first = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
    if first == len(a) == len(b):
        print("identical")
        return 0
    print(f"first change at line {first + 1}")
    for i in range(max(0, first - 3), min(max(len(a), len(b)), first + 7)):
        x = a[i] if i < len(a) else ""
        y = b[i] if i < len(b) else ""
        print(f"{'  ' if x == y else '- '}{x}" if x == y else f"- {x}\n+ {y}")
    ka, kb = kinds(a), kinds(b)
    for k in sorted(set(ka) | set(kb)):
        if ka[k] != kb[k]:
            print(f"{k}: {ka[k]} -> {kb[k]} ({kb[k] - ka[k]:+d})")
    return 1


if __name__ == "__main__":
    sys.exit(main())

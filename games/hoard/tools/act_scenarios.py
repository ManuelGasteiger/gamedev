#!/usr/bin/env python3
"""The 15 stress-test scenarios of act-api.md section 5 as acts in a lab world (item 09).

  act_scenarios.py --bin build/hoard-sim --data data [--seed 7] [--years 600] [--out out/act_scenarios]

A played run makes the lab world; each scenario sets its precondition on a copy (tags, a hostage, a relic, a visitor,
growth and talents so the gates hold) and runs every option as prefix + option through `hoard-sim --act-script`.
Prints one line per option and the scenarios with an option the sim refuses. Exit 1 when one cannot be expressed.
"""
import argparse
import copy
import json
import os
import subprocess
import sys


def act(type_, target=None, payload=None, secondary=None, magnitude=None, source="scenario", answers=None):
    a = {"type": type_, "source": source}
    if target: a["target"] = {"kind": target[0], "id": target[1]}
    if payload is not None: a["payload"] = payload
    if secondary: a["secondary"] = {"kind": secondary[0], "id": secondary[1]}
    if magnitude is not None: a["magnitude"] = magnitude
    if answers: a["answers"] = answers
    return a


class Lab:
    def __init__(self, world):
        self.w = world
        self.owner = {r["id"]: r.get("owner", "") for r in world["regions"]}
        self.adj = {r["id"]: r.get("neighbours", []) for r in world["regions"]}

    def people(self, court, role=None):
        return [x for x in self.w["people"] if x["faction"] == court and (role is None or x["role"] == role)]

    def neighbours(self, court):
        out = set()
        for r in self.w["regions"]:
            if r.get("owner") != court: continue
            for n in r.get("neighbours", []):
                o = self.owner.get(n, "")
                if o and o != court: out.add(o)
        return sorted(out)

    def region(self, court):
        return next(r["id"] for r in self.w["regions"] if r.get("owner") == court)

    def wild(self):
        return next((r["id"] for r in self.w["regions"] if not r.get("owner") and not r.get("lair")), None)

    def pick(self):
        best = None
        for f in self.w["factions"]:
            c = f["id"]
            if not self.people(c, "heir") or not self.people(c, "ruler") or not self.neighbours(c): continue
            score = len(self.people(c)) + len(f["regions"])
            if not best or score > best[0]: best = (score, c)
        return best[1]


def prepare(w):
    d = w["dragon"]
    for axis, n in (("might", 30), ("flame", 10), ("arcana", 20), ("lore", 20), ("dominion", 10)):
        d["growth"][axis] = max(d["growth"].get(axis, 0), n)
    for t in ("patient_mind", "old_tongues", "dragon_sight", "mortal_guise", "cult_of_the_mountain", "keen_eyes", "earth_shaker"):
        if t not in d["talents"]: d["talents"].append(t)
    d["magic"], d["knowledge"], d["faith_pool"] = max(d["magic"], 20), max(d["knowledge"], 20), max(d["faith_pool"], 20)
    d["size"] = max(d["size"], 10)


def faction(w, cid):
    return next(f for f in w["factions"] if f["id"] == cid)


def person(w, pid):
    return next(x for x in w["people"] if x["id"] == pid)


def add_hero(w, court):
    base = next(x for x in w["people"] if x["faction"] == court)
    h = copy.deepcopy(base)
    h["id"] = base["race"] + "9" + str(len(w["people"]))
    h["role"] = "hero"
    h["tags"] = []
    w["people"].append(h)
    w["people"].sort(key=lambda x: x["id"])
    return h["id"]


def cow(w, court):
    faction(w, court)["dread"]["oral"] = 0.6


def scenarios(lab):
    A = lab.pick()
    B = lab.neighbours(A)[0]
    R = lab.region(A)
    H = lab.people(A, "heir")[0]["id"]
    K = lab.people(A, "ruler")[0]["id"]
    out = []

    def sc(n, name, setup, prefix, options):
        out.append((n, name, setup, prefix, options))

    def s1(w): person(w, H)["tags"].append("lettered")
    sc(1, "the scribe among the hostages", s1, [act("take_hostage", ("faction", A), magnitude=0.2)],
       [[act("keep", ("person", H), {"role": "historian"}, source="lair")], [act("release", ("faction", A))], [act("kill", ("person", H), magnitude=0.3)]])

    def s2(w): next(r for r in w["regions"] if r["id"] == R)["tags"].append("library")
    sc(2, "a burning library", s2, [act("burn", ("region", R), secondary=("structure", "library"), magnitude=0.3, source="raid")],
       [[act("give", ("faction", A), {"books": 1})], [act("ignore", ("scenario", "s02"))], [act("teach", ("faction", A), {"subject": "letters"})]])

    hero = {}
    def s3(w):
        faction(w, A)["tags"].append("slayers")
        cow(w, A)
        hero["id"] = add_hero(w, A)
    sc(3, "the hero at dawn", s3, [],
       [[act("fight", ("person", lambda: hero["id"]))], [act("haunt", ("person", lambda: hero["id"]), {"message": "nightmare"})],
        [act("demand", ("faction", A), {"clause": "disband"})]])

    def s4(w): w.setdefault("secrets", []).append({"holder": A, "about": H, "kind": "corrupted:dark", "year": w["year"], "exposed": 0})
    sc(4, "the princess returns", s4,
       [act("take_hostage", ("faction", A), magnitude=0.2), act("teach", ("person", H), {"subject": "intrigue"}, source="lair"),
        act("corrupt", ("person", H), {"toward": "dark"}, source="lair"), act("release", ("faction", A))],
       [[act("reveal", ("faction", A), {"secret": "plot"})], [act("ignore", ("scenario", "s04"))], [act("guise", ("faction", A), {"purpose": "spy"})]])

    def s5(w): faction(w, A)["tags"].append("inquisition:" + H)
    sc(5, "the inquisition", s5, [],
       [[act("give", ("faction", A), {"artifacts": 1})], [act("judge", ("person", K), {"verdict": "spare"})],
        [act("incite", ("faction", B), secondary=("faction", A), source="lair")]])

    def s6(w): cow(w, A)
    sc(6, "the thief with the cup", s6, [],
       [[act("take", ("region", R), {"item": "cup"}, magnitude=0.1)], [act("demand", ("faction", A), {"clause": "hostage"})],
        [act("give", ("faction", A), {"gold": 5})]])

    terms = {"parties": ["dragon", A], "clauses": [{"who": "them", "pred": "no_harm"}]}
    sc(7, "two crowns", None, [act("take_hostage", ("faction", A), magnitude=0.2)],
       [[act("broker", ("faction", A), {"pact": "marriage"}, secondary=("faction", B))], [act("release", ("faction", A), {"terms": terms})],
        [act("ignore", ("scenario", "s07"))]])

    def s8(w):
        priest = (lab.people(A, "priest") or lab.people(A))[0]["id"]
        w["arrivals"] = [{"id": "arrival_lab_pilgrim", "kind": "pilgrim", "court": A, "person": priest, "year": w["year"], "because": []}]
    sc(8, "pilgrims in winter", s8, [],
       [[act("receive", payload={"outcome": "receive", "who": "arrival_lab_pilgrim"}, source="lair")],
        [act("receive", payload={"outcome": "eat", "who": "arrival_lab_pilgrim"}, source="lair")],
        [act("bless", ("region", R), {"kind": "land"}, source="lair")]])

    relic = "grave_crown:" + A
    def s9(w):
        w["dragon"].setdefault("chronicle", {}).setdefault("relics", []).append(
            {"id": relic, "kind": "grave_crown", "agenda": "lichdom", "court": A, "year": w["year"], "quiet_until": -1, "asked": -1, "answer": ""})
    sc(9, "the lich-crown speaks", s9, [],
       [[act("commune", ("item", relic), {"answer": "accept"}, source="lair")], [act("commune", ("item", relic), {"answer": "refuse"}, source="lair")],
        [act("give", ("faction", A), {"artifacts": 1})]])

    def s10(w): next(r for r in w["regions"] if r["id"] == R)["tags"].append("temple")
    sc(10, "a door in the temple", s10, [],
       [[act("consecrate", ("region", R))], [act("destroy_altar", ("region", R))], [act("defend", ("region", R), secondary=("faction", B), magnitude=1.0)]])

    sc(11, "the dwarves ask for fire", None, [],
       [[act("teach", ("faction", A), {"subject": "craft"})], [act("give", ("faction", A), {"artifacts": 1})], [act("ignore", ("scenario", "s11"))]])

    sc(12, "the quiet valley", None, [],
       [[act("show", ("faction", A), {"kind": "land"}, magnitude=0.5)], [act("guise", ("faction", A), {"purpose": "preach"})],
        [act("sleep", payload={"years": 500}, source="sleep")]])

    def s13(w): cow(w, A)
    sc(13, "the mountain", s13, [],
       [[act("terraform", ("region", lab.wild() or R), {"kind": "mountain"}, source="lair")], [act("sleep", payload={"years": 3000}, source="sleep")],
        [act("demand", ("faction", A), {"clause": "pilgrimage"})]])

    court = [act("guise", ("faction", A), {"purpose": "court", "person": H}), act("keep", ("person", H), {"role": "consort"}, source="lair")]
    sc(14, "the consort", None, [],
       [court, court + [act("dismiss", ("person", H), source="lair")], court + [act("release", ("faction", A))]])

    def s15(w):
        person(w, K)["tags"].append("false_dragon")
        faction(w, A)["tags"].append("false_dragon:" + K)
    sc(15, "the false dragon", s15, [],
       [[act("judge", ("person", K), {"verdict": "guilty"})], [act("guise", ("faction", A), {"purpose": "preach"})],
        [act("miracle", ("faction", A), {"kind": "protector"}, magnitude=1.0)]])
    return A, B, out


def resolve(a):
    a = copy.deepcopy(a)
    t = a.get("target")
    if t and callable(t["id"]): t["id"] = t["id"]()
    return a


def run(args, world, acts, tag):
    path = os.path.join(args.out, f"{tag}.json")
    json.dump({"world": world, "acts": acts}, open(path, "w"))
    res = subprocess.run([args.bin, "--data", args.data, "--act-script", path], capture_output=True, text=True, check=True)
    return [json.loads(l) for l in res.stdout.splitlines() if l.strip()]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default="build/hoard-sim")
    ap.add_argument("--data", default="data")
    ap.add_argument("--seed", type=int, default=7)
    ap.add_argument("--years", type=int, default=600)
    ap.add_argument("--out", default="out/act_scenarios")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    lab_dir = os.path.join(args.out, "lab")
    subprocess.run([args.bin, "--data", args.data, "--seed", str(args.seed), "--years", str(args.years), "--genesis", "--policy", "played",
                    "--out", lab_dir], check=True, stdout=subprocess.DEVNULL)
    base = json.load(open(os.path.join(lab_dir, "world_final.json")))
    prepare(base)
    A, B, cases = scenarios(Lab(base))
    print(f"lab world: seed {args.seed}, year {base['year']}, court {A}, neighbour {B}")
    failed = []
    for n, name, setup, prefix, options in cases:
        for k, option in enumerate(options):
            w = copy.deepcopy(base)
            if setup: setup(w)
            acts = [resolve(a) for a in prefix + option]
            lines = run(args, w, acts, f"s{n:02d}-{k}")
            bad = [l for l in lines if not l["accepted"]]
            what = " + ".join(a["type"] for a in option)
            if bad:
                print(f"{n:2d} {name}: {what}: refused {bad[0]['type']} ({bad[0].get('reason', '')})")
                failed.append((n, name, what, bad[0]["type"], bad[0].get("reason", "")))
            else:
                print(f"{n:2d} {name}: {what}: accepted")
    scen = sorted({f[0] for f in failed})
    print(f"scenarios with a refused option: {len(scen)} of {len(cases)}" + (": " + ", ".join(str(s) for s in scen) if scen else ""))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

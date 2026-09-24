#!/usr/bin/env python3
"""Summarize the Phase 2 matrix: one line per run, compared with the baseline of equal length."""
import csv, glob, os, sys
out = sys.argv[1] if len(sys.argv) > 1 else "experiments/out/phase2"
def load(d): return list(csv.DictReader(open(os.path.join(d, "metrics.csv"))))
def ang(a, b): return (a - b + 180.0) % 360.0 - 180.0
base = {}
for b in ("baseline", "baseline720"):
    if os.path.exists(os.path.join(out, b, "metrics.csv")):
        rows_b = load(os.path.join(out, b)); base[len(rows_b)] = rows_b
print(f"{'run':44} {'max|dmass|':>10} {'max|dhead|':>10} {'max dspread':>11} {'max|dspeed|':>11} {'final dmass':>11} {'recover':>8} {'end':>8}")
for d in sorted(glob.glob(os.path.join(out, "*"))):
    name = os.path.basename(d)
    if name.startswith("baseline"): continue
    rows = load(d); b = base.get(len(rows))
    if b is None: print(name, "no baseline of equal length"); continue
    mx = {"m": 0.0, "h": 0.0, "s": 0.0, "v": 0.0}; last_on = None; rec = None; extinct = None
    for r, br in zip(rows, b):
        step = int(r["step"]); on = float(r["stim_mean"]) > 0
        dm = float(r["A_sum"]) - float(br["A_sum"]); dh = ang(float(r["A_heading_deg"]), float(br["A_heading_deg"]))
        ds = float(r["A_spread"]) - float(br["A_spread"]); dv = float(r["A_speed"]) - float(br["A_speed"])
        if float(r["A_sum"]) < 1e-3 and extinct is None: extinct = step
        if on:
            last_on = step
            mx["m"] = max(mx["m"], abs(dm)); mx["h"] = max(mx["h"], abs(dh)); mx["s"] = max(mx["s"], ds, key=abs) if False else (ds if abs(ds) > abs(mx["s"]) else mx["s"]); mx["v"] = max(mx["v"], abs(dv))
        elif last_on is not None and rec is None and abs(dm) < 0.5 and abs(dh) < 2.0:
            rec = step - last_on
    end = "EXTINCT@%d" % extinct if extinct else ("ok" if rec is not None else "not-rec")
    print(f"{name:44} {mx['m']:10.2f} {mx['h']:10.2f} {mx['s']:11.3f} {mx['v']:11.3f} {dm:11.3f} {str(rec) if rec is not None else '-':>8} {end:>8}")

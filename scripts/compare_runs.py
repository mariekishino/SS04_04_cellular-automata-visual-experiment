#!/usr/bin/env python3
"""Compare a stimulated run against a silent baseline, snapshot by snapshot.

usage: scripts/compare_runs.py BASE_DIR STIM_DIR [--eps-mass 0.5] [--eps-heading 2.0]
Both runs must use the same --every. Prints per-snapshot differences and:
  - the largest deviation while the stimulus is on,
  - recovery: steps after the stimulus ends until mass and heading are back within eps of the baseline.
"""
import csv, math, sys

def load(d):
    return list(csv.DictReader(open(d + "/metrics.csv")))

def ang_diff(a, b):
    d = (a - b + 180.0) % 360.0 - 180.0
    return d

def main():
    args = [x for x in sys.argv[1:] if not x.startswith("--")]
    opts = dict(zip(sys.argv[1::1], sys.argv[2::1]))
    eps_mass = float(opts.get("--eps-mass", 0.5)); eps_head = float(opts.get("--eps-heading", 2.0))
    base, stim = load(args[0]), load(args[1])
    n = min(len(base), len(stim))
    print(f"{'step':>7} {'stim':>6} {'dmass':>8} {'dspeed':>8} {'dhead':>8} {'dspread':>8} {'mass':>8} {'comp':>4}")
    max_dev = {"dmass": 0.0, "dspeed": 0.0, "dhead": 0.0, "dspread": 0.0}
    last_on = None
    recovered_at = None
    extinct_at = None
    for i in range(n):
        b, s = base[i], stim[i]
        step = int(s["step"]); stv = float(s.get("stim_mean", 0.0))
        dm = float(s["A_sum"]) - float(b["A_sum"])
        dv = float(s["A_speed"]) - float(b["A_speed"])
        dh = ang_diff(float(s["A_heading_deg"]), float(b["A_heading_deg"]))
        ds = float(s["A_spread"]) - float(b["A_spread"])
        if float(s["A_sum"]) < 1e-3 and extinct_at is None: extinct_at = step
        if stv > 0:
            last_on = step
            for k, v in (("dmass", dm), ("dspeed", dv), ("dhead", dh), ("dspread", ds)):
                if abs(v) > abs(max_dev[k]): max_dev[k] = v
        elif last_on is not None and recovered_at is None and abs(dm) < eps_mass and abs(dh) < eps_head:
            recovered_at = step
        if i < 3 or stv > 0 or (last_on is not None and step - last_on <= 20 * int(s["step"]) / max(1, i)) or i == n - 1 or i % max(1, n // 12) == 0:
            print(f"{step:7d} {stv:6.3f} {dm:8.3f} {dv:8.3f} {dh:8.2f} {ds:8.3f} {float(s['A_sum']):8.2f} {int(s['A_components']):4d}")
    print("--- largest deviation while stimulus on:", {k: round(v, 3) for k, v in max_dev.items()})
    if extinct_at is not None: print(f"--- EXTINCT at step {extinct_at}")
    elif last_on is None: print("--- no stimulus in this run")
    elif recovered_at is None: print(f"--- NOT recovered by the end (last stimulus at step {last_on}); final dmass {dm:.3f} dhead {dh:.2f}")
    else: print(f"--- recovered {recovered_at - last_on} steps after the last stimulus (step {last_on} -> {recovered_at}), eps mass {eps_mass} heading {eps_head} deg")

if __name__ == "__main__":
    main()

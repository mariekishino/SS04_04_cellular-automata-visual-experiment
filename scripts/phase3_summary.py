#!/usr/bin/env python3
"""Phase 3 summary: probe response per run = max mass during the probe window minus mass just before it.

usage: scripts/phase3_summary.py [OUT=experiments/out/phase3]
"""
import csv, glob, json, os, re, sys
out = sys.argv[1] if len(sys.argv) > 1 else "experiments/out/phase3"

def probe_response(d):
    cfg = json.load(open(os.path.join(d, "config.json")))
    # the probe is the LAST pulse in the composite (added last); a continuous history pulse
    # has the same "period=0 count=1" form, so take the final match, not the first
    ms = re.findall(r"pulse start=([\d.]+) duration=([\d.]+) period=0 count=1", cfg["input_sequence"])
    if not ms: return None
    t0 = float(ms[-1][0]); sps = cfg["stimulus"]["steps_per_second"]
    s0 = int(t0 * sps); s1 = int((t0 + 1.0) * sps)
    rows = list(csv.DictReader(open(os.path.join(d, "metrics.csv"))))
    before = [r for r in rows if int(r["step"]) <= s0]
    during = [r for r in rows if s0 < int(r["step"]) <= s1]
    if not before or not during: return None
    base = float(before[-1]["A_sum"]); peak = max(float(r["A_sum"]) for r in during)
    m_at = float(before[-1].get("adapt_m", 0.0))
    g_at = float(before[-1].get("plast_g", float("nan")))
    return {"resp": peak - base, "m": m_at, "g": g_at, "base": base}

rows = []
for d in sorted(glob.glob(os.path.join(out, "*"))):
    name = os.path.basename(d)
    if name.startswith("shape_"): continue
    r = probe_response(d)
    if r: rows.append((name, r))

print(f"{'run':28} {'m@probe':>8} {'g@probe':>8} {'response':>9}")
for name, r in rows: print(f"{name:28} {r['m']:8.3f} {r['g']:8.4f} {r['resp']:9.2f}")

# matrix view: response by history x tau x silence
print("\nprobe response (mass gain) — rows: history, cols: silence 2/5/20/60 s")
for tau in ("2", "10", "60"):
    print(f"\n  tau {tau} s")
    print(f"  {'hist':6} {'2 s':>7} {'5 s':>7} {'20 s':>7} {'60 s':>7}   {'control k=0 (2 s)':>18}")
    for h in ("H0", "H1", "H2", "H3"):
        vals = []
        for sil in ("2", "5", "20", "60"):
            v = dict(rows).get(f"{h}_tau{tau}_s{sil}")
            vals.append(f"{v['resp']:7.2f}" if v else "      -")
        c = dict(rows).get(f"control_{h}_k0_s2")
        print(f"  {h:6} {' '.join(vals)}   {c['resp'] if c else float('nan'):18.2f}")

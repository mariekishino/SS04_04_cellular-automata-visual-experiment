#!/usr/bin/env bash
# Spontaneous-Orbium survey: run the `blobs` init for seeds 1..N and summarize.
# usage: scripts/seed_sweep.sh [N=20] [STEPS=1000] [OUT=experiments/out/sweep]
set -euo pipefail
n="${1:-20}"; steps="${2:-1000}"; out="${3:-experiments/out/sweep}"
mkdir -p "$out"
for ((s = 1; s <= n; s++)); do
  ./build/cave --quiet --model lenia --preset orbium --init blobs --seed "$s" --width 64 --height 64 \
    --steps "$steps" --every 50 --out "$out/seed_$(printf %02d "$s")"
done
python3 - "$out" "$n" <<'PY'
import csv, sys, glob, os
out, n = sys.argv[1], int(sys.argv[2])
print(f"{'seed':>4} {'mass_end':>9} {'above_end':>9} {'comp_end':>8} {'spread':>7} {'speed':>7}  verdict")
alive = 0
for d in sorted(glob.glob(os.path.join(out, "seed_*"))):
    rows = list(csv.DictReader(open(os.path.join(d, "metrics.csv"))))
    last = rows[-1]
    mass = float(last["A_sum"]); above = int(last["A_count_above"])
    comp = int(last.get("A_components", -1)); spread = float(last.get("A_spread", float("nan")))
    speed = float(last.get("A_speed", float("nan")))
    # verdict rule (explicit): one component, mass within 60..85 (Orbium ~71), still moving
    orb = (comp == 1 and 60.0 <= mass <= 85.0 and speed > 0.05)
    verdict = "orbium-like" if orb else ("extinct" if mass < 1e-3 else "other")
    alive += orb
    print(f"{os.path.basename(d)[5:]:>4} {mass:9.2f} {above:9d} {comp:8d} {spread:7.2f} {speed:7.3f}  {verdict}")
print(f"orbium-like: {alive}/{n}")
PY

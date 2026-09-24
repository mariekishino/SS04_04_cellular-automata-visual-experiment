#!/usr/bin/env bash
# Phase 2 stimulus matrix on the Orbium (64x64, periodic, 60 steps/s time base).
#   baseline : silence
#   short    : one 0.5 s pulse at t=2 s           (30 steps)
#   long     : one 10 s pulse at t=2 s            (600 steps)
#   entry x shape x gain, then WAV runs.
# usage: scripts/phase2_matrix.sh [OUT=experiments/out/phase2]
set -euo pipefail
out="${1:-experiments/out/phase2}"; B=./build/cave
common=(--quiet --model lenia --preset orbium --init orbium --width 64 --height 64 --every 20 --sps 60)
mkdir -p "$out"
$B "${common[@]}" --steps 1200 --out "$out/baseline"
for entry in growth mu; do
  if [ "$entry" = growth ]; then gains="0.05 0.1 0.2 0.5 1.0"; else gains="0.005 0.01 0.02 0.05 0.1"; fi
  for shape in uniform gradient_x; do
    for g in $gains; do
      $B "${common[@]}" --steps 1200 --out "$out/short_${entry}_${shape}_g${g}" --stim pulse --stim-mode "$entry" --stim-gain "$g" --stim-shape "$shape" --pulse-start 2 --pulse-dur 0.5 --pulse-count 1
      $B "${common[@]}" --steps 1200 --out "$out/long_${entry}_${shape}_g${g}"  --stim pulse --stim-mode "$entry" --stim-gain "$g" --stim-shape "$shape" --pulse-start 2 --pulse-dur 10 --pulse-count 1
    done
  done
done
# WAV: 8 s files + 4 s silent tail = 720 steps
for wav in kick120 tone; do
  for entry in growth mu; do
    if [ "$entry" = growth ]; then g=0.2; else g=0.01; fi
    for shape in uniform gradient_x; do
      $B "${common[@]}" --steps 720 --out "$out/wav_${wav}_${entry}_${shape}_g${g}" --stim wav --wav "experiments/audio/$wav.wav" --stim-mode "$entry" --stim-gain "$g" --stim-shape "$shape" --smooth 0.05
    done
  done
done
$B "${common[@]}" --steps 720 --out "$out/baseline720"
echo "MATRIX DONE"

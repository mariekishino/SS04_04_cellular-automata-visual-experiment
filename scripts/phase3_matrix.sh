#!/usr/bin/env bash
# Phase 3: history x time constant x silence length, then a common probe.
#   histories (0-20 s):  H0 silence | H1 sparse (0.5 s every 4 s, x5) | H2 dense (0.5 s every 1 s, x19) | H3 continuous (18 s)
#   common silence:      2 / 5 / 20 / 60 s   (probe at 22 / 25 / 40 / 80 s)
#   probe:               0.5 s, amplitude 1, growth uniform gain 0.3
#   adaptation:          k=3 with tau 2 / 10 / 60 s ; control k=0 (probe at 22 s)
#   extras:              k sweep (1/3/10) at tau 10 s, dense, 2 s ; gradient_x vs cos_x (10 s pulse, gain 0.2, no adaptation)
# usage: scripts/phase3_matrix.sh [OUT=experiments/out/phase3]
set -euo pipefail
out="${1:-experiments/out/phase3}"; B=./build/cave
common=(--quiet --model lenia --preset orbium --init orbium --width 64 --height 64 --every 30 --sps 60 --stim-mode growth --stim-gain 0.3)
mkdir -p "$out"
hist_args() {
  case "$1" in
    H0) echo "--stim none" ;;
    H1) echo "--stim pulse --pulse-start 1 --pulse-dur 0.5 --pulse-period 4 --pulse-count 5" ;;
    H2) echo "--stim pulse --pulse-start 1 --pulse-dur 0.5 --pulse-period 1 --pulse-count 19" ;;
    H3) echo "--stim pulse --pulse-start 1 --pulse-dur 18 --pulse-period 0 --pulse-count 1" ;;
  esac
}
run() { local name=$1 probe=$2 k=$3 tau=$4 hist=$5; local steps=$(( (${probe%.*} + 5) * 60 ));
  # shellcheck disable=SC2046
  $B "${common[@]}" --steps "$steps" --out "$out/$name" $(hist_args "$hist") --probe-at "$probe" --probe-dur 0.5 --probe-amp 1 --adapt-k "$k" --adapt-tau "$tau"; }
for hist in H0 H1 H2 H3; do
  run "control_${hist}_k0_s2" 22 0 10 "$hist"
  for tau in 2 10 60; do
    for sil in 2 5 20 60; do
      run "${hist}_tau${tau}_s${sil}" $((20 + sil)) 3 "$tau" "$hist"
    done
  done
done
for k in 1 10; do run "ksweep_H2_tau10_k${k}_s2" 22 "$k" 10 H2; done
# shape comparison (no adaptation): 10 s pulse from t=2, gain 0.2
for shape in gradient_x cos_x; do
  $B --quiet --model lenia --preset orbium --init orbium --width 64 --height 64 --every 20 --sps 60 --steps 1200 --out "$out/shape_${shape}" \
     --stim pulse --pulse-start 2 --pulse-dur 10 --pulse-count 1 --stim-mode growth --stim-gain 0.2 --stim-shape "$shape"
done
$B --quiet --model lenia --preset orbium --init orbium --width 64 --height 64 --every 20 --sps 60 --steps 1200 --out "$out/shape_baseline"
echo "MATRIX DONE"

#!/usr/bin/env bash
# Phase 4: environment (2 min) x common silence x plasticity on/off, then a common probe.
#   environments (0-120 s): E0 quiet | E1 dense (0.5 s every 1 s, x118) | E2 continuous (118 s)
#   common silence:         30 / 120 / 300 s  (probe at 150 / 240 / 420 s)
#   probe:                  0.5 s, amplitude 1, growth uniform, stim_gain 0.3
#   adaptation always on:   k=3, tau 10 s
#   plasticity:             off (rate 0) | on (rate 0.01 /s, return 180 s) ; rate sweep 0.003 / 0.03 at E2, 30 s
# usage: scripts/phase4_matrix.sh [OUT=experiments/out/phase4]
set -euo pipefail
out="${1:-experiments/out/phase4}"; B=./build/cave
common=(--quiet --model lenia --preset orbium --init orbium --width 64 --height 64 --every 30 --sps 60 --stim-mode growth --stim-gain 0.3 --adapt-k 3 --adapt-tau 10)
mkdir -p "$out"
env_args() {
  case "$1" in
    E0) echo "--stim none" ;;
    E1) echo "--stim pulse --pulse-start 1 --pulse-dur 0.5 --pulse-period 1 --pulse-count 118" ;;
    E2) echo "--stim pulse --pulse-start 1 --pulse-dur 118 --pulse-period 0 --pulse-count 1" ;;
  esac
}
run() { local name=$1 probe=$2 rate=$3 ret=$4 env=$5; local steps=$(( (probe + 5) * 60 ));
  # shellcheck disable=SC2046
  $B "${common[@]}" --steps "$steps" --out "$out/$name" $(env_args "$env") --probe-at "$probe" --probe-dur 0.5 --probe-amp 1 --plast-rate "$rate" --plast-return "$ret"; }
for env in E0 E1 E2; do
  for sil in 30 120 300; do
    run "off_${env}_s${sil}" $((120 + sil)) 0 180 "$env"
    run "on_${env}_s${sil}"  $((120 + sil)) 0.01 180 "$env"
  done
done
for rate in 0.003 0.03; do run "rate${rate}_E2_s30" 150 "$rate" 180 E2; done
run "return60_E2_s120" 240 0.01 60 E2
run "return600_E2_s120" 240 0.01 600 E2
echo "MATRIX DONE"

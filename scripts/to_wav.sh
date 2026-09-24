#!/usr/bin/env bash
# Convert any audio file ffmpeg can read (mp3, m4a, ogg, ...) to the WAV
# format the simulator reads: 44.1 kHz, mono, 16-bit PCM.
# usage: scripts/to_wav.sh input.mp3 [output.wav]
set -euo pipefail
in="${1:?input audio file}"
out="${2:-${in%.*}.wav}"
ffmpeg -loglevel error -y -i "$in" -ac 1 -ar 44100 -sample_fmt s16 "$out"
echo "wrote $out"

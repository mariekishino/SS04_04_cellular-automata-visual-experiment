#!/usr/bin/env bash
# Turn a run directory's frames into a video and a labeled contact sheet.
# usage: scripts/make_media.sh RUN_DIR [SHEET_FRAMES=12] [FPS=20]
set -euo pipefail
dir="${1:?run dir}"; n="${2:-12}"; fps="${3:-20}"
frames=("$dir"/frames/frame_*.png)
count=${#frames[@]}
[ "$count" -gt 0 ] || { echo "no frames in $dir"; exit 1; }
ffmpeg -loglevel error -y -framerate "$fps" -pattern_type glob -i "$dir/frames/frame_*.png" \
  -c:v libx264 -pix_fmt yuv420p -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" "$dir/video.mp4"
# pick n evenly spaced frames for the sheet
pick=()
for ((i = 0; i < n; i++)); do
  idx=$(( i * (count - 1) / (n - 1 > 0 ? n - 1 : 1) ))
  pick+=("${frames[$idx]}")
done
montage -label '%t' "${pick[@]}" -tile 6x -geometry +4+4 -background '#202020' -fill white -pointsize 14 "$dir/sheet.png"
echo "wrote $dir/video.mp4 and $dir/sheet.png ($count frames)"

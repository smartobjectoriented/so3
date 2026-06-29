#!/bin/bash
# Render every page of so3.drawio to a PNG (so3_<name>.png).
# The drawio CLI is an Electron app, so we run it under a virtual X server.
set -e
cd "$(dirname "$0")"

names=(modes architecture boot memory exception_levels syscall avz capsule device_model build io)

i=0
for name in "${names[@]}"; do
    # drawio's -p page selector is 1-based
    xvfb-run -a drawio -x -f png --scale 2 -p "$((i+1))" \
        -o "so3_${name}.png" so3.drawio \
        --no-sandbox --disable-gpu >/dev/null 2>&1 || true
    if [ -f "so3_${name}.png" ]; then
        echo "  [$i] so3_${name}.png  ($(stat -c%s "so3_${name}.png") bytes)"
    else
        echo "  [$i] FAILED: so3_${name}.png"
    fi
    i=$((i+1))
done

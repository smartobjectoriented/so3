#!/bin/sh

# Copyright (c) 2025-2026 EDGEMTech SA

# Launch the EDGE-M1 customized Toradex Easy Installer (auto feed-add).
# Build the FIT first with: scripts/tezi-custom/build.sh

set -e
DIR=$(cd "$(dirname "$0")/tezi-custom" && pwd)
if [ ! -f "$DIR/tezi.itb" ]; then
    echo "tezi-custom/tezi.itb missing — run scripts/tezi-custom/build.sh first" >&2
    exit 1
fi
cd "$DIR"
sudo ./recovery/uuu recovery

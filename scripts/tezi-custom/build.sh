#!/usr/bin/env bash

# Copyright (c) 2025-2026 EDGEMTech SA

# Build a customized Toradex Easy Installer (tezi.itb) for the verdin-imx8mp
# board: same as the upstream Toradex one, with /etc/rc.local extended to
# auto-register a local nginx image feed (see rc.local.append.in).
#
# Zero-config defaults:
#   - host IP   detected via `ip route get 1.1.1.1` (override: EDGEM_HOST_IP)
#   - upstream  downloaded from tezi.toradex.com into build/tmp/tezi-installer/
#               (override: TEZI_SOURCE_DIR — pre-existing local archive root)
#
# Output: tezi.itb in this directory + symlinks/copies of unchanged upstream
# artefacts. Launcher: scripts/recovery-edgem.sh

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
WORK=${WORK:-/tmp/tezi-custom-build}

TEZI_VERSION=${TEZI_VERSION:-7.5.0+build.9}
TEZI_RELEASE=${TEZI_RELEASE:-9}
TEZI_ARCHIVE_URL=${TEZI_ARCHIVE_URL:-"https://tezi.toradex.com/artifactory/tezi-oe-prod-frankfurt/scarthgap-7.x.y/release/${TEZI_RELEASE}/verdin-imx8mp/tezi/tezi-run/oedeploy/Verdin-iMX8MP_ToradexEasyInstaller_${TEZI_VERSION}.zip"}
TEZI_CACHE_DIR=${TEZI_CACHE_DIR:-"$PROJECT_ROOT/build/tmp/tezi-installer"}
TEZI_SOURCE_DIR=${TEZI_SOURCE_DIR:-}

for t in dumpimage unsquashfs mksquashfs fakeroot sha256sum python3 curl unzip ip; do
    command -v "$t" >/dev/null || { echo "missing tool: $t" >&2; exit 1; }
done

if [ -z "$TEZI_SOURCE_DIR" ]; then
    TEZI_SOURCE_DIR="$TEZI_CACHE_DIR/Verdin-iMX8MP_ToradexEasyInstaller_$TEZI_VERSION"
    if [ ! -f "$TEZI_SOURCE_DIR/tezi.itb" ]; then
        echo "Toradex Easy Installer ${TEZI_VERSION} not in cache — downloading..."
        mkdir -p "$TEZI_CACHE_DIR"
        TMP_ZIP="$TEZI_CACHE_DIR/.tezi-${TEZI_VERSION}.zip"
        curl -fL --retry 3 -o "$TMP_ZIP" "$TEZI_ARCHIVE_URL"
        rm -rf "$TEZI_SOURCE_DIR"
        unzip -q -d "$TEZI_CACHE_DIR" "$TMP_ZIP"
        rm "$TMP_ZIP"
        if [ ! -f "$TEZI_SOURCE_DIR/tezi.itb" ]; then
            echo "Archive extracted but tezi.itb not at expected path: $TEZI_SOURCE_DIR" >&2
            echo "Inspect $TEZI_CACHE_DIR and set TEZI_SOURCE_DIR to the right directory." >&2
            exit 1
        fi
    fi
elif [ ! -f "$TEZI_SOURCE_DIR/tezi.itb" ]; then
    echo "TEZI_SOURCE_DIR=$TEZI_SOURCE_DIR does not contain tezi.itb" >&2
    exit 1
fi
echo "Using upstream TEZI at: $TEZI_SOURCE_DIR"

if [ -z "${EDGEM_HOST_IP:-}" ]; then
    # Walk default routes in metric order; skip tunnel-like interfaces (vpn0,
    # tun*, wg*, ...) so an active VPN doesn't capture the board-flashing IP.
    while IFS= read -r dev; do
        case "$dev" in vpn*|tun*|tap*|wg*|ppp*) continue ;; esac
        EDGEM_HOST_IP=$(ip -4 -o addr show dev "$dev" 2>/dev/null \
            | awk 'NR==1 {sub(/\/.*/,"",$4); print $4}')
        [ -n "$EDGEM_HOST_IP" ] && break
    done < <(ip -4 route show default | awk '{for(i=1;i<=NF;i++) if($i=="dev") print $(i+1)}')
fi
if [ -z "${EDGEM_HOST_IP:-}" ]; then
    echo "Could not auto-detect host IP. Set EDGEM_HOST_IP=<ip> manually." >&2
    exit 1
fi
# Port 8080 by default: the feed is served straight out of the build tree by
# scripts/tezi-feed-serve.sh, running as your user, so it cannot bind :80.
# Keep this in step with IB_HTTP_FEED_PORT in build/conf/local.conf — the URL
# is baked into the TEZI image here, so a port change means rebuilding it.
EDGEM_FEED_PORT=${EDGEM_FEED_PORT:-8080}
EDGEM_FEED_URL=${EDGEM_FEED_URL:-"http://${EDGEM_HOST_IP}:${EDGEM_FEED_PORT}/image_list.json"}
echo "Embedded feed URL: $EDGEM_FEED_URL"

VENV=$SCRIPT_DIR/.venv
if ! "$VENV/bin/python" -c 'import libfdt' 2>/dev/null; then
    python3 -m venv "$VENV"
    "$VENV/bin/pip" -q install pylibfdt
fi

rm -rf "$WORK"
mkdir -p "$WORK"

sed "s|@EDGEM_FEED_URL@|$EDGEM_FEED_URL|g" "$SCRIPT_DIR/rc.local.append.in" > "$WORK/rc.local.append"

dumpimage -T flat_dt -p 29 -o "$WORK/initramfs.squashfs" "$TEZI_SOURCE_DIR/tezi.itb" >/dev/null

fakeroot bash -c "
    set -e
    cd '$WORK'
    unsquashfs -d rootfs initramfs.squashfs >/dev/null
    cat rc.local.append >> rootfs/etc/rc.local
    mksquashfs rootfs new.sqfs -comp gzip -b 131072 -noappend -no-progress >/dev/null
"

"$VENV/bin/python" "$SCRIPT_DIR/patch_fit.py" \
    "$TEZI_SOURCE_DIR/tezi.itb" \
    "$WORK/new.sqfs" \
    "$SCRIPT_DIR/tezi.itb"

ln -sfn "$TEZI_SOURCE_DIR/boot.scr"              "$SCRIPT_DIR/boot.scr"
ln -sfn "$TEZI_SOURCE_DIR/imx-boot-recoverytezi" "$SCRIPT_DIR/imx-boot-recoverytezi"
ln -sfn "$TEZI_SOURCE_DIR/imx-boot-sd"           "$SCRIPT_DIR/imx-boot-sd"
ln -sfn "$TEZI_SOURCE_DIR/overlays.txt"          "$SCRIPT_DIR/overlays.txt"

# recovery/ must be a real subdirectory: when uuu resolves the relative paths
# in uuu.auto, the kernel walks `recovery/../<file>` and if `recovery/` were a
# symlink to upstream Toradex it would end up loading the unmodified upstream
# tezi.itb. Only the uuu binaries are symlinked. uuu also unconditionally
# prepends the cmdlist's directory to each `-f` path, so absolute paths break
# with "recovery//home/...". Keep them relative.

rm -rf "$SCRIPT_DIR/recovery"
mkdir -p "$SCRIPT_DIR/recovery"
ln -sfn "$TEZI_SOURCE_DIR/recovery/uuu"     "$SCRIPT_DIR/recovery/uuu"
ln -sfn "$TEZI_SOURCE_DIR/recovery/uuu.exe" "$SCRIPT_DIR/recovery/uuu.exe"
cp "$TEZI_SOURCE_DIR/recovery/uuu.auto"     "$SCRIPT_DIR/recovery/uuu.auto"

echo
echo "tezi-custom built (feed=$EDGEM_FEED_URL). Launch with: scripts/recovery-edgem.sh"

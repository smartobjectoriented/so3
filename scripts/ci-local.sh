#!/usr/bin/env bash
#
# ci-local.sh — run the GitHub "Build" workflow (.github/workflows/build.yml)
# locally, without pushing.
#
# The CI checks out the repository with actions/checkout (only git-TRACKED
# files, a fresh tree with no build output) and then, inside the so3-env
# container, appends IB_PLATFORM and runs `build.sh -x so3` + `build.sh -x
# usr-so3`. To match that faithfully this script exports the tracked files
# into a throwaway directory and runs the exact same docker command.
#
# Why a throwaway copy instead of mounting the repo directly:
#   - untracked files are EXCLUDED, so "I forgot to `git add` a patch" fails
#     here exactly like it does in CI (instead of silently passing);
#   - your real build/tmp is untouched, and the container builds from scratch
#     (fresh timestamps), reproducing the toolchain build the CI actually does.
# Uncommitted edits to tracked files ARE included, so you can test before you
# commit. Use -r <ref> to test an exact committed state (git archive) instead.
#
# Usage:
#   scripts/ci-local.sh                 # both platforms (virt32, virt64)
#   scripts/ci-local.sh -p virt32       # one platform
#   scripts/ci-local.sh -p virt32,virt64
#   scripts/ci-local.sh -r HEAD         # test committed HEAD exactly (byte-for-byte)
#   scripts/ci-local.sh -k              # keep the temp dir even on success
#
set -euo pipefail

IMAGE="ghcr.io/smartobjectoriented/so3-env:latest"
PLATFORMS="virt32,virt64"
REF=""          # empty => working-tree tracked files; non-empty => git archive of REF
KEEP=0

# Where the throwaway trees are created. This MUST live under $HOME: snap- and
# rootless-packaged Docker daemons are confined and can only bind-mount paths
# under the user's home — a /tmp mount silently shows up EMPTY in the container
# (the daemon has its own /tmp namespace). Override with CI_LOCAL_WORKDIR.
WORKBASE="${CI_LOCAL_WORKDIR:-$HOME/.cache/so3-ci-local}"

usage() { sed -n '2,40p' "$0"; exit "${1:-0}"; }

while getopts ":p:r:kh" opt; do
  case "$opt" in
    p) PLATFORMS="$OPTARG" ;;
    r) REF="$OPTARG" ;;
    k) KEEP=1 ;;
    h) usage 0 ;;
    *) usage 1 ;;
  esac
done

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

# Export the tree to copy into the container mount.
populate() {
  local dest="$1"
  if [ -n "$REF" ]; then
    git archive --format=tar "$REF" | tar -C "$dest" -xf -
  else
    # tracked files with current (working-tree) content; build/tmp & other
    # gitignored output are not tracked, so they are naturally excluded.
    git ls-files -z | tar --null -C "$ROOT" -cf - -T - | tar -C "$dest" -xf -
  fi
}

# The build runs as root inside the container, so it leaves root-owned files in
# the work tree that the host user cannot delete (rm: "Permission denied"). Hand
# ownership back to the invoking user via the same image (root) before the host
# removes or inspects the tree.
reown() {
  docker run --rm -v "$1:/w" "$IMAGE" chown -R "$(id -u):$(id -g)" /w 2>/dev/null || true
}

rc=0
IFS=',' read -ra PLATS <<< "$PLATFORMS"
for plat in "${PLATS[@]}"; do
  [ -n "$plat" ] || continue
  src_desc="${REF:-working tree (tracked files)}"
  echo "============================================================"
  echo "[ci-local] platform=$plat  source=$src_desc  image=$IMAGE"
  echo "============================================================"

  mkdir -p "$WORKBASE"
  work="$(mktemp -d "$WORKBASE/$plat.XXXXXX")"
  populate "$work"

  # Exact replica of the build.yml step (IB_PLATFORM appended, then the builds).
  if docker run --rm -v "$work:/so3" "$IMAGE" bash -c '
        set -e
        cd /so3
        echo "IB_PLATFORM = \"'"$plat"'\"" >> build/conf/local.conf
        . ./env.sh
        build.sh -x so3
        build.sh -x usr-so3
      '; then
    echo "[ci-local] platform=$plat: PASS"
    reown "$work"
    [ "$KEEP" -eq 1 ] || rm -rf "$work"
  else
    rc=1
    reown "$work" # so the kept tree can be inspected / removed without sudo
    echo "[ci-local] platform=$plat: FAIL  (tree kept for inspection: $work)"
  fi
done

echo "============================================================"
[ "$rc" -eq 0 ] && echo "[ci-local] ALL PASSED" || echo "[ci-local] FAILURES (exit $rc)"
exit "$rc"

#!/bin/sh

# Run an Infrabase command inside the build container.
#
# The container provides the ENVIRONMENT (cross toolchains, host
# packages, CMake, Python); the repository stays on the host and is
# bind-mounted, so sources, build/tmp and the produced images remain
# exactly where they are and stay owned by the calling user.
#
# Usage:
#   dbuild.sh --build              Build (or rebuild) the image
#   dbuild.sh                      Interactive shell inside the container
#   dbuild.sh build.sh bsp-so3     Run a build
#   dbuild.sh deploy.sh bsp-so3    Deploy it
#   dbuild.sh st.sh -d             Run it under QEMU, graphical
#
# The command runs with the project root bind-mounted at its OWN
# absolute path and the current directory preserved, so a tree can be
# built from inside or outside the container interchangeably: bitbake
# stamps, CMake caches and the *.attach.sha256 manifests all record
# absolute paths and would otherwise be invalidated on every switch.
#
# Environment overrides:
#   IB_DOCKER_IMAGE   image name:tag        (default: so3-build:1.0)
#   IB_DOCKER_OPTS    extra `docker run` options
#
# Copyright (c) 2026 REDS Institute - HEIG-VD

set -e

progname=$(basename "$0")

IB_DOCKER_IMAGE=${IB_DOCKER_IMAGE:-so3-build:1.0}

# Resolve the project root from this script's own location so dbuild.sh
# works from anywhere inside the tree.
#
# `pwd -P` (physical path) on purpose: a tree is commonly reached through
# a symlink (~/edgemtech/edgem1 -> products/edgem1/edgem1), and the cwd
# check below and the bind mount both work on plain strings. Without
# resolving, entering by the symlink while the script resolves the real
# path aborts with "current directory is outside <tree>".

IB_ROOT=$(cd "$(dirname "$(command -v -- "$0")")/.." && pwd -P)
CONTEXT="$IB_ROOT/docker/build-env"

pr_usage()
{
	printf "Run an Infrabase command inside the build container\n\n"
	printf "Usage: %s [--build] [<command> [args...]]\n\n" "$progname"
	printf "    --build     Build (or rebuild) the %s image\n" "$IB_DOCKER_IMAGE"
	printf "    <command>   Command to run inside the container (default: an\n"
	printf "                interactive shell). env.sh is sourced first, so\n"
	printf "                scripts/ and bitbake are on PATH.\n\n"
	printf "Examples:\n"
	printf "    %s --build\n" "$progname"
	printf "    %s build.sh bsp-so3\n" "$progname"
	printf "    %s deploy.sh bsp-so3\n" "$progname"
	printf "    %s st.sh -d\n" "$progname"
}

if ! command -v docker >/dev/null 2>&1; then
	printf "%s: docker is not installed\n" "$progname" >&2
	exit 1
fi

case "$1" in
	-h|--help)
		pr_usage
		exit 0
		;;
	--build)
		printf "[dbuild] building %s from %s\n" "$IB_DOCKER_IMAGE" "$CONTEXT"

		# Context is docker/build-env only — never the project root,
		# which carries tens of GB of build output.

		exec docker build -t "$IB_DOCKER_IMAGE" "$CONTEXT"
		;;
esac

if ! docker image inspect "$IB_DOCKER_IMAGE" >/dev/null 2>&1; then
	printf "%s: image '%s' not found — run: %s --build\n" \
		"$progname" "$IB_DOCKER_IMAGE" "$progname" >&2
	exit 1
fi

# The cwd is reused verbatim inside the container and only $IB_ROOT is
# mounted, so it has to live inside the tree.

cwd=$(pwd -P)
case "$cwd" in
	"$IB_ROOT"|"$IB_ROOT"/*) ;;
	*)
		printf "%s: current directory is outside %s — cd into the tree first\n" \
			"$progname" "$IB_ROOT" >&2
		exit 1
		;;
esac

# Build the docker argv by PREPENDING to the user command, so the
# command always stays last and no quoting is lost.
#
# No command: an interactive shell driven by the image's rc file, which
# sources env.sh IN that shell so its cd/pushd/popd wrappers and
# ib_autoswitch_* functions exist (a parent that sources env.sh then
# execs bash would pass on the variables but lose the functions).
#
# With a command: source env.sh first, because the front-end scripts
# refuse to reconfigure the environment in a non-interactive context.

if [ $# -eq 0 ]; then
	set -- "$IB_DOCKER_IMAGE" bash --rcfile /etc/so3-build.bashrc -i
else
	set -- "$IB_DOCKER_IMAGE" bash -c \
		'cd "$2" && . ./env.sh >/dev/null 2>&1; cd "$1"; shift 2; exec "$@"' \
		dbuild "$cwd" "$IB_ROOT" "$@"
fi

set -- -e IB_TREE="$IB_ROOT" -e IB_CWD="$cwd" "$@"

# Hardware deployment: make any IB_HTTP_DEPLOY_PATH feed directory
# visible at its own path so `deploy.sh` can publish into it from inside
# (IB_STORAGE_MODE=http). This serves the verdin-imx8mp TEZI flow; on the
# soft/hard storage platforms it is a no-op.
# IB_STORAGE_MODE=hard needs no extra wiring: /dev is already
# bind-mounted and the container is privileged (it writes the HOST's
# device, so double-check IB_STORAGE_DEVICE).
#
# The DEFAULT feed lives inside the tree (IB_HTTP_DEPLOY_PATH in
# local.conf), which is already bind-mounted, so this loop does nothing
# for it. It only matters for a build/conf/site.conf that redirects the
# feed out of the tree.
#
# A snap-packaged Docker cannot do this: the confined daemon only reaches
# $HOME (and a few allowed paths), so bind-mounting e.g. /var/www/html
# fails with "mkdir /var/www: read-only file system". Skip the mount
# there rather than making every invocation fail, and say so once.

_snap_docker=0
case "$(command -v docker)" in
	/snap/*) _snap_docker=1 ;;
esac

# The client path is not proof either way: the docker snap also ships a
# /usr/bin/docker wrapper, so a confined daemon can hide behind an
# ordinary-looking binary. Ask the daemon itself — a snap daemon keeps
# its root under /var/snap. Only worth doing when the path check missed.

if [ "$_snap_docker" = "0" ]; then
	case "$(docker info --format '{{.DockerRootDir}}' 2>/dev/null)" in
		/var/snap/*|/snap/*) _snap_docker=1 ;;
	esac
fi

for _feed in $(sed -n 's/^[[:space:]]*IB_HTTP_DEPLOY_PATH[^=]*=[[:space:]]*"\([^"]*\)".*/\1/p' \
		"$IB_ROOT/build/conf/local.conf" "$IB_ROOT/build/conf/site.conf" \
		2>/dev/null | sort -u); do
	# Tree-relative defaults are already inside the bind-mounted tree, and
	# ${...} references are not expanded here — skip both.
	case "$_feed" in
		*'${'*)            continue ;;
		"$IB_ROOT"|"$IB_ROOT"/*) continue ;;
	esac

	[ -d "$_feed" ] || continue

	case "$_feed" in
		"$HOME"/*) ;;
		*)
			if [ "$_snap_docker" = "1" ]; then
				printf '%s: skipping bind mount of %s (snap Docker cannot mount outside $HOME) — publish the TEZI feed from the host, or install Docker from the apt repository\n' \
					"$progname" "$_feed" >&2
				continue
			fi
			;;
	esac

	set -- -v "$_feed:$_feed" "$@"
done

# Forward the ssh-agent when present, so a git fetch over SSH (private
# submodules) works from inside. Nothing is copied into the image.

if [ -n "$SSH_AUTH_SOCK" ] && [ -S "$SSH_AUTH_SOCK" ]; then
	set -- -e SSH_AUTH_SOCK="$SSH_AUTH_SOCK" \
		-v "$SSH_AUTH_SOCK:$SSH_AUTH_SOCK" "$@"
fi

# Forward the X11 display for the graphical QEMU launcher (st.sh -d).
#
# The X cookie must come along with the socket: the container user has a
# different HOME, so ~/.Xauthority would not be found, and under Wayland
# the cookie lives outside HOME anyway (XWayland puts it in
# /run/user/<uid>/). Mount the file at its own path and point XAUTHORITY
# at it; without this the GTK window dies with "Authorization required".

if [ -n "$DISPLAY" ] && [ -d /tmp/.X11-unix ]; then
	set -- -e DISPLAY="$DISPLAY" -v /tmp/.X11-unix:/tmp/.X11-unix "$@"

	_xauth=${XAUTHORITY:-$HOME/.Xauthority}
	if [ -f "$_xauth" ]; then
		set -- -e XAUTHORITY="$_xauth" -v "$_xauth:$_xauth:ro" "$@"
	fi
fi

# Always keep stdin attached (-i): without it Docker gives the container
# /dev/null, so piping into a containerised command silently delivers
# nothing — e.g. `printf 'cmd\n' | dbuild.sh st.sh` never reaches the
# guest console. Harmless when there is no input: the command just sees
# EOF. Allocate a tty (-t) only for a real terminal, so CI logs stay
# clean.

if [ -t 0 ] && [ -t 1 ]; then
	set -- -it "$@"
else
	set -- -i "$@"
fi

# shellcheck disable=SC2086
[ -n "$IB_DOCKER_OPTS" ] && set -- $IB_DOCKER_OPTS "$@"

# --privileged + /dev: the storage steps (build.sh filesystem, deploy.sh)
# need loop devices, mount and mkfs. Loop devices are a HOST-kernel
# resource, so concurrent builds on one machine contend for them exactly
# as they do outside a container.
#
# --network host: keeps st.sh's slirp port forwards (guest ssh on 2222,
# or the first free port above it), the GDB stub and the Sense HAT bridge
# (4442) reachable from the host
# without publishing ports, and lets the recipes fetch through the host
# resolver.

set -- --rm \
	--privileged \
	--network host \
	-v /dev:/dev \
	-v "$IB_ROOT:$IB_ROOT" \
	-w "$cwd" \
	-e HOST_UID="$(id -u)" \
	-e HOST_GID="$(id -g)" \
	-e TERM="${TERM:-xterm}" \
	"$@"

docker run "$@"
_rc=$?

# The TEZI feed server has to run on the HOST: the container is --rm, so a
# server started inside it dies with the command that started it. It shares
# the host network namespace (--network host above), but not its lifetime.
#
# Run this after the container rather than before, so the very first
# `dbuild.sh deploy.sh ...` — which creates the feed inside the container —
# leaves a serving feed behind too. Idempotent, and a no-op when nothing has
# been published yet or a server is already up.

"$IB_ROOT/scripts/tezi-feed-serve.sh" --ensure

exit $_rc

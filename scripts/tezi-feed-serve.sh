#!/bin/sh

# Serve the TEZI HTTP feed produced by `deploy.sh` so a board can pull it
# during a TEZI network auto-install.
#
# The feed directory (IB_HTTP_DEPLOY_PATH) lives inside the tree and is
# self-contained: deploy.sh writes image_list.json next to image.json, so
# this script only has to expose that directory as the document root. The
# board's feed URL is therefore always:
#
#     http://<host-ip>:<port>/image_list.json
#
# The server runs as the calling user on an unprivileged port — no root, no
# web server configuration, no document-root permissions to arrange, and
# nothing written outside the tree.
#
# Copyright (c) 2025-2026 EDGEMTech SA

# Usage:
#   tezi-feed-serve.sh              Serve in the foreground (Ctrl-C to stop)
#   tezi-feed-serve.sh --ensure     Start detached if not already running
#   tezi-feed-serve.sh --status     Report whether it runs, and on which URL
#   tezi-feed-serve.sh --stop       Stop a detached server
#
# --ensure is what build.sh/deploy.sh/dbuild.sh call, so a normal deploy
# leaves a working feed behind with no manual step. It is idempotent and
# silent when the server is already up.

progname=$(basename "$0")

IB_ROOT=$(cd "$(dirname "$(command -v -- "$0")")/.." && pwd -P)

pr_usage()
{
	printf "Serve the TEZI HTTP feed for network auto-install\n\n"
	printf "Usage: %s [--ensure|--status|--stop|-h]\n" "$progname"
}

pr_help()
{
	printf "\nAvailable options:\n"
	printf "    -h, --help                Print this help\n"
	printf "    --ensure                  Start detached unless already running\n"
	printf "    --status                  Report state and feed URL\n"
	printf "    --stop                    Stop a detached server\n"
	printf "\nWith no option the server runs in the foreground.\n"
	printf "\nThe feed directory and port come from build/conf/local.conf,\n"
	printf "overridable per machine in build/conf/site.conf:\n"
	printf "    IB_HTTP_DEPLOY_PATH, IB_HTTP_FEED_PORT\n"
}

# --- configuration ----------------------------------------------------------
#
# Read a bitbake variable the way the front-end scripts do: last matching
# assignment wins, site.conf after local.conf so a per-machine override
# takes precedence. Only plain `=`/`?=`/`:=` assignments are understood,
# which is all these two variables ever use.

conf_get()
{
	_var=$1
	_val=""
	for _conf in "$IB_ROOT/build/conf/local.conf" "$IB_ROOT/build/conf/site.conf"; do
		[ -f "$_conf" ] || continue
		_hit=$(sed -n "s/^[[:space:]]*${_var}[[:space:]]*[?:]*=[[:space:]]*\"\([^\"]*\)\".*/\1/p" \
			"$_conf" | tail -1)
		[ -n "$_hit" ] && _val=$_hit
	done
	printf '%s' "$_val"
}

# IB_PLATFORM is scoped-override aware in check-env.sh; here the bare
# assignment is enough because IB_HTTP_DEPLOY_PATH interpolates it.
platform=$(sed -n 's/^[[:space:]]*IB_PLATFORM[[:space:]]*??*=[[:space:]]*"\([^"]*\)".*/\1/p' \
	"$IB_ROOT/build/conf/local.conf" 2>/dev/null | tail -1)

feed=$(conf_get IB_HTTP_DEPLOY_PATH)
port=$(conf_get IB_HTTP_FEED_PORT)
[ -n "$port" ] || port=8080

# Expand the two bitbake variables the default value uses. Anything else
# left unexpanded means the override is beyond what this parser handles.
feed=$(printf '%s' "$feed" | sed -e "s|\${IB_DIR}|$IB_ROOT|g" -e "s|\${IB_PLATFORM}|$platform|g")

case "$feed" in
	"")   printf "%s: IB_HTTP_DEPLOY_PATH not found in build/conf/*.conf\n" "$progname" >&2; exit 1 ;;
	*'${'*) printf "%s: cannot expand IB_HTTP_DEPLOY_PATH (%s)\n" "$progname" "$feed" >&2; exit 1 ;;
esac

runtime_dir="$IB_ROOT/build/deploy"
pidfile="$runtime_dir/tezi-feed.pid"
logfile="$runtime_dir/tezi-feed.log"

# --- helpers ----------------------------------------------------------------

# Host IP as the board sees it: first default route, skipping tunnel-like
# interfaces so an active VPN does not capture the flashing address. This is
# the address printed below and handed to the board with `tezictl feed-add`,
# so a wrong pick sends the board to a host it cannot reach.
host_ip()
{
	ip -4 route show default 2>/dev/null \
		| awk '{for (i = 1; i <= NF; i++) if ($i == "dev") print $(i + 1)}' \
		| while IFS= read -r dev; do
			case "$dev" in vpn*|tun*|tap*|wg*|ppp*) continue ;; esac
			ip -4 -o addr show dev "$dev" 2>/dev/null \
				| awk 'NR == 1 {sub(/\/.*/, "", $4); print $4; exit}'
			break
		done
}

feed_url()
{
	_ip=$(host_ip)
	[ -n "$_ip" ] || _ip="<host-ip>"
	printf 'http://%s:%s/image_list.json' "$_ip" "$port"
}

# A live pidfile means our server; a stale one is cleaned up so --ensure
# can restart after a reboot or a kill without manual intervention.
running_pid()
{
	[ -f "$pidfile" ] || return 1
	_pid=$(cat "$pidfile" 2>/dev/null)
	case "$_pid" in ''|*[!0-9]*) rm -f "$pidfile"; return 1 ;; esac
	if kill -0 "$_pid" 2>/dev/null; then
		printf '%s' "$_pid"
		return 0
	fi
	rm -f "$pidfile"
	return 1
}

# --- actions ----------------------------------------------------------------

do_status()
{
	if _pid=$(running_pid); then
		printf "running (pid %s)  %s\n" "$_pid" "$(feed_url)"
		printf "  feed: %s\n" "$feed"
		printf "  log : %s\n" "$logfile"
		return 0
	fi
	printf "not running  (feed: %s)\n" "$feed"
	return 1
}

do_stop()
{
	if _pid=$(running_pid); then
		kill "$_pid" 2>/dev/null
		rm -f "$pidfile"
		printf "%s: stopped (pid %s)\n" "$progname" "$_pid"
		return 0
	fi
	printf "%s: not running\n" "$progname"
	return 0
}

# Serving an empty or missing directory would answer 404 to the board and
# look like a network fault, so say plainly that a deploy has to run first.
check_feed()
{
	if [ ! -d "$feed" ]; then
		printf "%s: feed directory does not exist yet: %s\n" "$progname" "$feed" >&2
		printf "%s: run a deploy first (e.g. deploy.sh bsp-linux)\n" "$progname" >&2
		return 1
	fi
	if [ ! -f "$feed/image_list.json" ]; then
		printf "%s: %s has no image_list.json — deploy has not published here yet\n" \
			"$progname" "$feed" >&2
		return 1
	fi
	return 0
}

do_serve_foreground()
{
	check_feed || exit 1
	printf "%s: serving %s\n" "$progname" "$feed"
	printf "%s: feed URL  %s\n" "$progname" "$(feed_url)"
	printf "%s: board cmd tezictl feed-add %s\n" "$progname" "$(feed_url)"
	exec python3 -m http.server "$port" --directory "$feed"
}

do_ensure()
{
	# Never leave a daemon behind on a CI runner: CI builds and deploys to
	# verify, it never flashes a board from its own feed.
	if [ -n "${CI:-}" ] || [ -n "${GITLAB_CI:-}" ]; then
		return 0
	fi

	# Inside the build container this is a no-op: the container is --rm, so
	# a server started here would die with it. dbuild.sh starts one on the
	# host before the container runs (it shares the host network namespace).
	if [ -f /.dockerenv ] || [ -n "${container:-}" ]; then
		return 0
	fi

	running_pid >/dev/null && return 0

	# Called after every build and deploy, including on platforms that never
	# publish a feed (soft/hard storage), so a missing feed is the normal
	# case here: stay quiet and leave the explaining to check_feed, which
	# only runs when the server is asked for explicitly.

	[ -d "$feed" ] && [ -f "$feed/image_list.json" ] || return 0

	command -v python3 >/dev/null 2>&1 || {
		printf "%s: python3 not found — cannot serve the feed\n" "$progname" >&2
		return 0
	}

	mkdir -p "$runtime_dir"

	# Detached and immune to the parent shell going away, so the feed
	# outlives the deploy that started it.
	nohup python3 -m http.server "$port" --directory "$feed" \
		>>"$logfile" 2>&1 &
	_pid=$!
	printf '%s\n' "$_pid" > "$pidfile"

	# Give it a moment to fail (port already taken by something else) so the
	# user hears about it now rather than when a board fails to flash.
	sleep 1
	if ! kill -0 "$_pid" 2>/dev/null; then
		rm -f "$pidfile"
		printf "%s: feed server failed to start on port %s — see %s\n" \
			"$progname" "$port" "$logfile" >&2
		printf "%s: another server on that port? override IB_HTTP_FEED_PORT in build/conf/site.conf\n" \
			"$progname" >&2
		return 0
	fi

	printf "[tezi-feed] serving %s on %s (pid %s)\n" "$feed" "$(feed_url)" "$_pid"
	return 0
}

# --- main -------------------------------------------------------------------

case "${1:-}" in
	-h|--help) pr_usage; pr_help; exit 0 ;;
	--ensure)  do_ensure; exit 0 ;;
	--status)  do_status; exit $? ;;
	--stop)    do_stop; exit $? ;;
	"")        do_serve_foreground ;;
	*)         pr_usage >&2; exit 1 ;;
esac

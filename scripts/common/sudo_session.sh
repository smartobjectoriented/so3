# Copyright (c) 2025-2026 EDGEMTech SA
#
# Sudo session helper: keep a sudo timestamp valid for the lifetime of
# the calling script so privileged operations issued from bitbake
# recipes (via `sudo -n`) never hit an interactive password prompt
# mid-build.
#
# bitbake itself runs as the unprivileged user — only individual
# privileged operations (mount/umount/losetup/mkfs/parted/...) are
# escalated, and each one is invoked with `sudo -n` so a missing
# timestamp fails fast rather than blocking on stdin.
#
# Usage in callers:
#   . scripts/common/sudo_session.sh
#   sudo_session_start || exit 1
#   # ... run bitbake / scripts that may invoke `sudo -n` ...
#   # keep-alive is auto-killed on EXIT/INT/TERM via the installed trap

sudo_session_start() {
	# Validate the sudo timestamp upfront. With a NOPASSWD sudoers
	# entry this is a silent no-op; otherwise the user gets a single
	# password prompt right here, BEFORE bitbake starts.
	if ! sudo -v
	then
		printf "Error: failed to acquire sudo credentials\n" >&2
		return 1
	fi

	# The keep-alive below refreshes the timestamp every 60 s so that
	# bitbake recipe tasks can use `sudo -n` throughout the build.
	# This only works when the sudo timestamp is UID-scoped (global),
	# not TTY-scoped (the default). Detect the common misconfiguration
	# and guide the user to the one-time fix.
	_ts_type=$(sudo -V 2>/dev/null | sed -n 's/.*Timestamp type: *//p')
	if [ -n "$_ts_type" ] && [ "$_ts_type" != "global" ]
	then
		printf "\nWARNING: sudo timestamp_type is '%s' (not global).\n" "$_ts_type" >&2
		printf "         bitbake recipe tasks run in subprocesses without a TTY\n" >&2
		printf "         and will fail with 'sudo: a password is required'.\n\n" >&2
		printf "         Fix (run once on this host):\n\n" >&2
		printf "             scripts/common/setup_sudo.sh\n\n" >&2
		printf "         Continuing anyway — the build may fail mid-task.\n\n" >&2
	fi
	unset _ts_type

	# Background poker: refresh the sudo timestamp every 60s. The
	# default sudo TTL is 5 min — 60s gives generous headroom while
	# staying cheap. The loop exits when the parent shell is gone
	# (kill -0 check) so we never leak a daemon if the script dies.
	_sudo_parent_pid=$$
	(
		while true; do
			sudo -n true 2>/dev/null || exit
			sleep 60
			kill -0 "$_sudo_parent_pid" 2>/dev/null || exit
		done
	) &
	_sudo_keepalive_pid=$!

	# Kill the keep-alive on any exit path (normal, ^C, kill).
	trap "kill $_sudo_keepalive_pid 2>/dev/null" EXIT INT TERM

	return 0
}

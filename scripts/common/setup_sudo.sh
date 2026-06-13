#!/bin/sh

# One-time host setup: configure sudo so that privileged operations
# invoked from bitbake recipes (via `sudo -n`) share the credential
# timestamp with the parent deploy/build shell.
#
# By default sudo uses tty_tickets, tying each cached password to a
# specific TTY. bitbake tasks run in subprocesses that may not inherit
# the caller's TTY, so `sudo -n` fails even after sudo_session_start
# has validated the password. Setting timestamp_type=global makes the
# credential UID-scoped and shared across all processes for this user.
#
# This script only needs to be run once per host. It writes to
# /etc/sudoers.d/ and requires sudo itself to do so.
#
# Usage:
#   scripts/common/setup_sudo.sh
#   scripts/common/setup_sudo.sh --check   (check only, no changes)

# Copyright (c) 2025-2026 EDGEMTech SA

set -e

SUDOERS_FILE="/etc/sudoers.d/infrabase-${USER}"
CHECK_ONLY=0

if [ "$1" = "--check" ]; then
    CHECK_ONLY=1
fi

_check_already_configured() {
    if [ -f "$SUDOERS_FILE" ]; then
        if grep -q "timestamp_type=global" "$SUDOERS_FILE" 2>/dev/null; then
            return 0
        fi
    fi
    # Also check if the main sudoers file already has timestamp_type=global
    if sudo grep -q "timestamp_type=global" /etc/sudoers 2>/dev/null; then
        return 0
    fi
    return 1
}

if _check_already_configured; then
    printf "sudo timestamp_type=global is already configured for %s.\n" "$USER"
    exit 0
fi

if [ "$CHECK_ONLY" = "1" ]; then
    printf "sudo is NOT configured for global timestamps (tty_tickets is active).\n"
    printf "Run:  scripts/common/setup_sudo.sh\n"
    printf "to fix it (requires your sudo password once).\n"
    exit 1
fi

printf "Configuring sudo for user '%s' to use global timestamp type.\n" "$USER"
printf "This allows bitbake recipe tasks to use 'sudo -n' after\n"
printf "sudo_session_start has validated your password.\n\n"
printf "You will be prompted for your sudo password once.\n\n"

# Write the sudoers snippet via a temp file, validate it, then install.
TMPFILE="$(mktemp)"
cat > "$TMPFILE" <<EOF
# infrabase: allow bitbake recipe tasks to use sudo -n without a TTY.
# timestamp_type=global makes the cached credential UID-scoped so all
# subprocesses (including those spawned by bitbake) share the session
# opened by sudo_session_start. timestamp_timeout keeps it alive long
# enough for the keep-alive loop (60s interval) in sudo_session.sh.
Defaults:${USER} timestamp_type=global,timestamp_timeout=30
EOF

# Validate with visudo before installing
if ! sudo visudo -c -f "$TMPFILE" 2>&1; then
    rm -f "$TMPFILE"
    printf "Error: sudoers syntax check failed. Aborting.\n" >&2
    exit 1
fi

sudo install -m 0440 -o root -g root "$TMPFILE" "$SUDOERS_FILE"
rm -f "$TMPFILE"

printf "Installed %s\n" "$SUDOERS_FILE"
printf "Done. Run 'scripts/deploy.sh' or 'scripts/build.sh' normally now.\n"

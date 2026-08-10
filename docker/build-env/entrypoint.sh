#!/bin/sh

# Container entrypoint: run the build as the HOST user, not as root.
#
# Everything the recipes write lands in the bind-mounted repository —
# source trees attached in place, build/tmp, filesystem images. If the
# container ran as root, all of it would come back root-owned on the
# host, reintroducing exactly the ownership churn the unprivileged
# bitbake model was designed to remove.
#
# The UID/GID cannot be baked into the image: they differ per host
# (1000 for a developer laptop, 996 for gitlab-runner on the CI box).
# So the entrypoint starts as root, materialises a matching account,
# then drops to it.
#
# Copyright (c) 2026 REDS Institute - HEIG-VD

set -e

HOST_UID=${HOST_UID:-1000}
HOST_GID=${HOST_GID:-1000}

# Reuse an existing group/user with that id when the base image already
# ships one (ubuntu:24.04 has 'ubuntu' at 1000) — useradd would fail on
# a duplicate id otherwise.

group_name=$(getent group "$HOST_GID" | cut -d: -f1)
if [ -z "$group_name" ]; then
	group_name=builder
	groupadd -g "$HOST_GID" "$group_name"
fi

user_name=$(getent passwd "$HOST_UID" | cut -d: -f1)
if [ -z "$user_name" ]; then
	user_name=builder
	useradd -m -u "$HOST_UID" -g "$HOST_GID" -s /bin/bash "$user_name"
fi

usermod -aG sudo "$user_name" 2>/dev/null || true

# A writable HOME matters: pip caches and ccache land there (the Sense
# HAT emulator venv lives in build/tmp, inside the bind-mounted tree).

user_home=$(getent passwd "$HOST_UID" | cut -d: -f6)
if [ ! -d "$user_home" ]; then
	mkdir -p "$user_home"
fi
chown "$HOST_UID:$HOST_GID" "$user_home"

export HOME="$user_home"
export USER="$user_name"

# setpriv (util-linux) rather than su/sudo: no extra process in the
# signal path, so Ctrl-C reaches the build and exit codes propagate.

exec setpriv --reuid="$HOST_UID" --regid="$HOST_GID" --init-groups "$@"

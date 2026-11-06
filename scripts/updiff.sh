#!/bin/sh

# Copyright (c) 2025-2026 EDGEMTech SA

# Release banner, once per invocation (see scripts/common/banner.sh).
. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/banner.sh"

# A BSP recipe only assembles what the component recipes produce: it has no
# source tree and no patch set to fold anything into.

case "$1" in
bsp-*)
	echo "updiff.sh: $1 has no patch set, run updiff on the component recipe" \
		"(linux, so3, usr-linux, usr-so3, uboot, ...)" >&2
	exit 1
	;;
esac

bitbake "$1" -c updiff


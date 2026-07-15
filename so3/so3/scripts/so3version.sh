#!/bin/sh
#
# Print the SO3 release version string used in the boot banner.
#
# The version is derived from the git release tag (e.g. "v6.2.1") so the
# banner always matches the current release without any manual bump. Only the
# base version is kept: any "-<commits>-g<hash>" / "-dirty" suffix added by
# git-describe on a post-tag (dev) commit is stripped, so both a tagged build
# and a dev build on top of v6.2.1 report "6.2.1".
#
# When the build tree carries no git metadata (e.g. a bitbake WORKDIR copy),
# fall back to the SO3_KERNEL_VERSION_FALLBACK constant baked into
# include/version.h.
#
# Usage: so3version.sh [srctree]
#

srctree="${1:-.}"

# Preferred source: the nearest reachable git tag.
if v=$(cd "$srctree" 2>/dev/null && git describe --tags 2>/dev/null) && [ -n "$v" ]; then
	# Drop the "-<commits>-g<hash>" and "-dirty" suffixes -> base tag only.
	v=$(printf '%s' "$v" | sed -e 's/-[0-9][0-9]*-g[0-9a-f]*//' -e 's/-dirty$//')
	# Drop a leading "v".
	printf '%s' "${v#v}"
	exit 0
fi

# Fallback: the constant checked into version.h.
sed -n 's/^#define[[:space:]]\{1,\}SO3_KERNEL_VERSION_FALLBACK[[:space:]]\{1,\}"\(.*\)".*/\1/p' \
	"$srctree/include/version.h"

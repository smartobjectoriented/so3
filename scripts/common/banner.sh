# Copyright (c) 2026 EDGEMTech SA

# Source me first thing from every user-facing script under scripts/.
#
# Prints which Infrabase release is running, on stderr so it never mixes
# with what a script writes on stdout:
#
#     [infrabase v1.0.0] deploy.sh bsp-linux
#
# Once per invocation, not once per script: scripts call each other
# (build.sh runs the scripts/bitbake wrapper, deploy.sh runs
# tezi-feed-serve.sh), so the first one to print exports IB_BANNER_SHOWN and
# the nested ones stay quiet. dbuild.sh forwards it into the container for the
# same reason.
#
# The version comes from scripts/ibversion.sh (the infrabase release this tree
# is aligned on).

if [ -z "${IB_BANNER_SHOWN:-}" ]; then
	_ib_banner_dir="$(cd "$(dirname "$(command -v -- "$0")")" && pwd)"

	printf '[infrabase v%s] %s%s\n' \
		"$(sh "$_ib_banner_dir/ibversion.sh")" \
		"$(basename "$0")" "${*:+ $*}" >&2

	IB_BANNER_SHOWN=1
	export IB_BANNER_SHOWN
	unset _ib_banner_dir
fi

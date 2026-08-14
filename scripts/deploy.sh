#!/bin/sh

# General deployment script for the infrabase infrastructure.
#
# bitbake itself runs as the unprivileged user. Privileged operations
# inside individual recipe tasks (mount/umount/losetup/mkfs/parted/...)
# escalate to root via `sudo -n` and rely on the sudo timestamp opened
# here by sudo_session_start.

# Copyright (c) 2014-2026 REDS Institute, HEIG-VD
# Copyright (c) 2023-2026 EDGEMTech

progname=$(basename $0)

# Resolve project root from this script's own location, cd there, and
# source env.sh — prompting the user first if the parent shell points
# at a different tree. See scripts/common/setup_env.sh for details.
. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/setup_env.sh"

pr_usage()
{
	printf "Infrabase deployment script\n\n"
	printf "Usage: $progname [-h] [-l] [-v] [-x] <recipe_name>\n"
}

pr_help()
{
	printf "\nAvailable options:\n"
	printf "    -h                        Print this help\n"
	printf "    -l                        List all deployable recipes\n"
	printf "    -x <recipe_name>          Deploy a recipe. -x is optional: the recipe may be\n"
	printf "                              given as a bare argument. A BSP recipe (e.g. bsp-so3)\n"
	printf "                              deploys the full image; a component (usr-so3, ...)\n"
	printf "                              deploys just its own part.\n"
	printf "    -v                        Emit logs during deployment\n"
	printf "Examples: \n\n"
	printf "$progname -l                  List all deployable recipes\n"
	printf "$progname usr-so3             Deploy the SO3 user space into the rootfs\n"
	printf "$progname -v bsp-so3          Deploy the full SO3 BSP with verbose logs\n"
}

if test $# -eq 0
then
	pr_usage
	printf "\nUse $progname -h to show help\n"
	exit 1
fi

. ./scripts/common/bblayers.sh
. ./scripts/common/sudo_session.sh

recipename=''
optverbose=0
dolist=0
dodeploy=0

# Options first, then the recipe as a positional argument. -x is accepted
# for explicitness/symmetry but is optional (`deploy.sh bsp-so3` works).
while getopts "hlvx" o; do
	case "$o" in
		h)
			# Help summary
			pr_usage
			pr_help
			exit
			;;
		x)
			# Optional "deploy this recipe" marker; the recipe is positional.
			;;
		l)
			dolist=1
			;;
		v)
			optverbose=1
			;;
		*)
			pr_usage;
			exit 1
			;;
	esac
done
shift $((OPTIND - 1))
recipename="$1"

if test -n "$recipename"
then
	deploytask="do_deploy"
	dodeploy=1
fi

show_env "$recipename"

if test -z "$recipename" && test $dolist -eq 0
then
	printf "Error: please specify a recipe name\n\n"
	pr_usage
	exit 1
fi

# deploy.sh inherits the bblayers/build state left by the prior
# `build.sh` invocation for the same recipe.

if test $dolist -eq 1
then
	echo "Listing ALL available deployable recipes:"

	recipes=$(available_recipes "")

	for r in $recipes
	do
		bitbake -c listtasks "$r" | grep "do_deploy" >/dev/null 2>&1
		if test $? -eq 0
		then
			# Recipe has a deployment task - list it
			echo $r
		fi
	done
	exit
fi

IB_BB_OPTS=''

if test $optverbose -eq 1
then
	IB_BB_OPTS='-vDDD'
fi

if test $dodeploy -eq 1
then
	printf "\n*** NOTE: *** Deployment may require root access for mount/losetup/\n"
	printf "mkfs/parted/... You may be prompted for the sudo password once.\n\n"

	# Acquire a sudo timestamp upfront and keep it warm for the entire
	# deploy. bitbake runs unprivileged below; recipes escalate via
	# `sudo -n` so a missing timestamp fails fast rather than blocking
	# on stdin mid-build. The keep-alive is killed on EXIT/INT/TERM by
	# a trap installed inside sudo_session_start.
	sudo_session_start || exit 1

	bitbake $recipename -c $deploytask $IB_BB_OPTS
	res=$?

	if test $res -ne 0
	then
		exit 1
	fi

	# Make the freshly published TEZI feed reachable without a manual
	# step. Idempotent and quiet when a server is already up; a no-op
	# when the deploy did not publish an HTTP feed, in CI, and inside
	# the build container (dbuild.sh does it on the host instead —
	# a server started in the container would die with it).
	./scripts/tezi-feed-serve.sh --ensure
fi

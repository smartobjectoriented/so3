#!/bin/sh

# General build script for the infrabase infrastructure.
#
# bitbake runs as the unprivileged user. The only build invocation that
# needs root (the filesystem image creation, `-x filesystem`) still runs
# bitbake unprivileged — its recipe calls `sudo -n` for the privileged ops
# (losetup/fdisk/mkfs) via the sudo timestamp opened by
# sudo_session_start.

# Copyright (c) 2014-2026 REDS Institute, HEIG-VD
# Copyright (c) 2023-2026 EDGEMTech

progname=$(basename $0)

# Resolve the project root from this script's own location, cd there,
# and source env.sh — prompting the user first if the parent shell is
# wired to a different tree. Lets build.sh be invoked from anywhere
# (sibling worktree, build/, etc.) without silently switching shells
# between trees.
. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/setup_env.sh"

pr_usage()
{
	printf "Infrabase build script\n\n"
	printf "Usage: $progname [-h] [-l] [-c] [-v] [-x] <recipe_name>\n"
}

pr_help()
{

	printf "\nAvailable options:\n"
	printf "    -h                           Print this help\n"
	printf "    -l                           List all available recipes (BSPs and components)\n"
	printf "    -x <recipe_name>             Build a recipe. -x is optional: the recipe may be\n"
	printf "                                 given as a bare argument. A BSP recipe (e.g.\n"
	printf "                                 bsp-linux) pulls its whole dependency tree; a\n"
	printf "                                 component (uboot, rootfs, ...) builds just itself.\n"
	printf "    -c                           Clean the recipe first, then rebuild\n"
	printf "    -v                           Emit verbose build logs\n\n"
	printf "Examples: \n\n"
	printf "$progname -l                              List all recipes\n"
	printf "$progname uboot                           Build u-boot\n"
	printf "$progname -x usr-so3                      Build the SO3 user space\n"
	printf "$progname bsp-so3                         Build the full SO3 BSP\n"
	printf "$progname -v -c bsp-so3                   Clean and rebuild the BSP, verbose\n"
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
dolist=0
dobuild=0
doclean=0
optverbose=0
rootprivs=0

# Options first, then the recipe as a positional argument. -x is accepted
# for explicitness/symmetry but is optional (`build.sh bsp-linux` works).
while getopts "chlvx" o; do
	case "$o" in
		l)
			dolist=1
			;;
		h)
			# Help summary
			pr_usage
			pr_help
			exit
			;;
		c)
			doclean=1
			;;
		x)
			# Optional "build this recipe" marker; the recipe is positional.
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
	dobuild=1
	# Some recipes need root at build time (loop-mount / losetup / mkfs /
	# cpio -id). Open a sudo session for them. bsp-linux pulls the rootfs
	# mount at build time (do_itb before do_build); filesystem creates the
	# image. Extend this match if more are added.
	case "$recipename" in
		bsp-linux*|filesystem) rootprivs=1 ;;
	esac
fi

show_env "$recipename"

if test -z "$recipename" && test $dolist -eq 0
then
	printf "Error: Please specify recipe name\n\n"
	pr_usage
	exit 1
fi

# List all available recipes (BSPs and components).
if test $dolist -eq 1
then
	printf "Listing ALL available recipes:\n"
	available_recipes ""
	exit
fi

IB_BB_OPTS=''

if test $optverbose -eq 1
then
	IB_BB_OPTS='-vDDD'
fi

# Repair stale recipe workdirs first. An interrupted task (e.g. Ctrl-C
# during a clean) can leave a recipe WORKDIR that exists but is missing
# its temp/ subdir; bitbake then can't create that task's fifo and fails
# with "No such file or directory: .../temp/fifo.NNNN" (typically on
# do_clean). Such a workdir holds nothing useful, so remove it and let
# bitbake recreate it cleanly.
if test -d "$BUILDDIR/tmp/work"
then
	for _wd in "$BUILDDIR"/tmp/work/*/
	do
		if test -d "$_wd" && ! test -d "${_wd}temp"
		then
			echo "[infrabase] removing stale workdir (no temp/): $_wd"
			rm -rf "$_wd"
		fi
	done
fi

if test $doclean -eq 1
then
	bitbake $recipename -c clean $IB_BB_OPTS
fi

if test $dobuild -eq 1
then
	if test $rootprivs -eq 1
	then
		printf "\n *** NOTE: *** '$recipename' invokes privileged tools\n"
		printf "You may be prompted for the sudo password once.\n\n"

		# Open a sudo session: validate timestamp upfront + keep alive
		# in the background. Recipes escalate individual commands via
		# `sudo -n`. bitbake itself stays unprivileged.
		sudo_session_start || exit 1
	fi

	bitbake $recipename $IB_BB_OPTS
fi

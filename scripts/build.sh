#!/bin/sh

# General build script for the infrabase infrastructure.
#
# bitbake runs as the unprivileged user. The only build invocation that
# needs root (the filesystem image creation, `-f`) still runs bitbake
# unprivileged — its recipe calls `sudo -n` for the privileged ops
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
	printf "Usage: $progname [-h] [-l] [-a|-b|-x|-k|-f|-r] <recipe_name> [-c][-v]\n"
}

pr_help()
{

	printf "\nAvailable options:\n"
	printf "    -h                           Print this help\n"
	printf "    -l                           List available BSPs, kernels, components\n"
	printf "    -a <bsp_recipe_name>         Build all, the name of BSP is required\n"
	printf "    -k <kernel_recipe_name>      Build kernel only\n"
	printf "    -x <component_recipe_name>   Build component or tool\n"
	printf "    -r <rootfs_recipe_name>      Build rootfs\n"
	printf "    -f                           Create and format filesystem image\n"
	printf "    -b                           Build uboot only\n"
	printf "                                 of the selected base BSP (e.g. fc, dev-lvgl)\n"
	printf "    -v                           Emit verbose build logs\n"
	printf "    -c                           Clean before rebuilding\n\n"
	printf "Examples: \n\n"
	printf "$progname -l                              Print all recipes\n"
	printf "$progname -l -a                           Print all BSP recipes\n"
	printf "$progname -l -k                           Print all kernel recipes\n"
	printf "$progname -a bsp-linux                    Build the bare bsp-linux BSP (no capsule)\n"
	printf "$progname -v -a bsp-linux -c              Clean and rebuild all emitting verbose logs\n"
}

if test $# -eq 0
then
	pr_usage
	printf "\nUse $progname -h to show help\n"
	exit 1
fi

. ./scripts/common/bblayers.sh
. ./scripts/common/sudo_session.sh

layernames=''
recipename=''
dolist=0
dobuild=0
doclean=0
optverbose=0
rootprivs=0

while getopts "abcfhklrvx" o; do
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
		a)
			if ! test -n "$2"
			then
				# List all recipes in 'meta-bsp'
				layernames="meta-bsp"
			else
				recipename="$2"
				dobuild=1
				# do_build is pure compilation for most BSPs, so `-a` needs
				# NO privileges. The privileged rootfs loop-mount lives in
				# usr-*:do_deploy, which a BSP pulls in via do_itb. Whether
				# that happens at *build* time depends on how the recipe
				# wires do_itb:
				#   - bsp-so3 / bsp-capsules: `do_itb before do_deploy` only
				#       -> do_build is privilege-free; the mount happens in
				#         deploy.sh. `build.sh -a` must NOT prompt for sudo.
				#   - bsp-linux: `do_itb before do_build` (so `-a` also
				#       produces the .itb) -> do_build DOES pull the mount and
				#         needs root at build time.
				# Only open a sudo session for the recipes that actually need
				# it at build time (extend this match if more are added).
				case "$recipename" in
					bsp-linux*) rootprivs=1 ;;
				esac
			fi
			;;
		r)
			if ! test -n "$2"
			then
				layernames="meta-rootfs"
			else
				recipename="$2"
				dobuild=1
			fi
			;;
		b)
			layernames="meta-uboot"
			recipename="uboot"
			dobuild=1
			;;
		x)
			if test -n "$2"
			then
				recipename="$2"
				dobuild=1
			else
				layernames="$IB_AUX_LAYERS"
			fi
			;;
		c)
			recipename="$2"
			doclean=1
			;;
		v)
			optverbose=1
			;;
		k)
			if ! test -n "$2"
			then
				layernames="meta-linux meta-so3"
			else
				recipename="$2"
				dobuild=1
			fi
			;;
		f)
			recipename="filesystem"
			rootprivs=1
			dobuild=1
			;;
		*)
			pr_usage;
			exit 1
			;;
	esac
done

show_env "$recipename"

if test -z $recipename && test $dolist -eq 0
then
	printf "Error: Please specify recipe name\n\n"
	pr_usage
	exit 1
fi

# The user is willing to list available recipes
# dolist action is available for component options

if test $dolist -eq 1
then
	if test -z "$layernames"
	then
		printf "Listing ALL available recipes:\n"
	else
		printf "Listing recipes in layer(s): $layernames\n"
	fi

	available_recipes "$layernames"
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

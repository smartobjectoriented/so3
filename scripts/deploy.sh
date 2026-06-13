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
	printf "Usage: $progname [-h] [-l] [-a|-b|-r|-x|-k] [-v] ... \n"
}

pr_help()
{
	printf "\nAvailable options:\n"
	printf "    -h                        Print this help\n"
	printf "    -l                        List available recipes per layer or globally\n"
	printf "    -a <bsp_recipe_name>      Deploy all, kernel, uboot, rootfs, usr\n"
	printf "    -k <bsp_recipe_name>      Deploy kernel (with ITB)\n"
	printf "    -b                        Deploy uboot\n"
	printf "    -r <rootfs_recipe_name>   Deploy specified rootfs\n"
	printf "    -x <aux_recipe_name>      Deploy auxiliary component\n"
	printf "    -v                        Emit logs during deployment\n"
	printf "Examples: \n\n"
	printf "$progname -l -a               List all deployable BSP recipes\n"
	printf "$progname -l -x               List all deployable auxiliary components\n"
	printf "$progname -b -v               Deploy uboot with verbose logs\n"
	printf "$progname -a bsp-linux -v     Deploy ALL rootfs, kernel, uboot with verbose logs\n"
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
layernames=''
optverbose=0
dolist=0
dodeploy=0

while getopts "abhklrvx" o; do
	case "$o" in
		h)
			# Help summary
			pr_usage
			pr_help
			exit
			;;
		a)
			if ! test -n "$2"
			then
				layernames="meta-bsp"
			else
				recipename="$2"
				deploytask="do_deploy"
				dodeploy=1
			fi
			;;
		k)
			if ! test -n "$2"
			then
				layernames="meta-bsp"
			else
				recipename="$2"
				deploytask="do_deploy_boot"
				dodeploy=1
			fi
			;;
		x)
			if ! test -n "$2"
			then
				layernames="$IB_AUX_LAYERS"
			else
				recipename="$2"
				deploytask="do_deploy"
				dodeploy=1
			fi
			;;
		r)
			if ! test -n "$2"
			then
				layernames="meta-rootfs"
			else
				recipename="$2"
				deploytask="do_deploy"
				dodeploy=1
			fi
			;;
		b)
			# We only currently have uboot for everything
			# So no optarg for -b
			layernames="meta-uboot"
			recipename="uboot"
			deploytask="do_deploy"
			dodeploy=1
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

show_env "$recipename"

if test -z $recipename && test $dolist -eq 0
then
	printf "Error: please specify a recipe name\n\n"
	pr_usage
	exit 1
fi

# deploy.sh inherits the bblayers/build state left by the prior
# `build.sh` invocation for the same recipe.

if test $dolist -eq 1
then
	if test -z "$layernames"
	then
		echo "Listing ALL available deployable recipes:"
	else
		echo "Listing available deployable recipes in layer(s): $layernames"
	fi

	recipes=$(available_recipes "$layernames")

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
fi
